#pragma once

// Shared "PWM sound engine" post-processing DSP for both emulator cores
// (UAE and vAmiga). Ported from the amiga-paula project's PWM renderer
// (pwm_paula.cpp): punch enhancement + room simulation.
//
// Both stages operate on the final stereo float stream and are engine
// agnostic. All time constants are derived from the actual output sample
// rate (the reference implementation was tuned at 48 kHz).

#include <array>
#include <cmath>
#include <cstdint>

namespace qsr_dsp {

// Punch enhancement: hybrid edge boost + envelope-gated transient boost.
// This is sound design, not emulation - it adds attack definition and
// harmonic richness beyond what stock Paula produces. Linear phase
// (uses only current and previous samples).
class PunchEnhancer {
public:
    void setup(double sampleRate) {
        // Reference constants tuned @48kHz; envelope release is a per-sample
        // decay coefficient, rescale by rate: coef = ref^(48000/rate)
        m_release = static_cast<float>(std::pow(0.998, 48000.0 / sampleRate));
        reset();
    }

    void reset() {
        m_prevL = m_prevR = 0.0f;
        m_envL = m_envR = 0.0f;
    }

    // In-place processing of one stereo sample pair.
    void process(float& l, float& r) {
        constexpr float edgeBlend = 0.08f;  // constant +6dB/oct tilt amount
        constexpr float attack = 0.3f;      // envelope attack speed
        constexpr float transBoost = 0.2f;  // transient expansion amount

        const float inL = l;
        const float inR = r;
        const float diffL = inL - m_prevL;
        const float diffR = inR - m_prevR;

        // Envelope follower on difference magnitude
        const float magL = std::fabs(diffL);
        const float magR = std::fabs(diffR);
        m_envL = (magL > m_envL) ? magL * attack + m_envL * (1.0f - attack) : m_envL * m_release;
        m_envR = (magR > m_envR) ? magR * attack + m_envR * (1.0f - attack) : m_envR * m_release;

        // Edge component (constant derivative blend)
        l += diffL * edgeBlend;
        r += diffR * edgeBlend;

        // Transient component (envelope-gated, activates on attacks)
        l += diffL * m_envL * transBoost;
        r += diffR * m_envR * transBoost;

        m_prevL = inL;
        m_prevR = inR;
    }

private:
    float m_prevL = 0.0f, m_prevR = 0.0f;
    float m_envL = 0.0f, m_envR = 0.0f;
    float m_release = 0.998f;
};

// Room simulation: delayed opposite-channel bleed with gentle ~10kHz lowpass
// (air absorption, not head shadow). Reduces headphone fatigue from the
// Amiga's hard L-R-R-L panning while preserving high-frequency clarity.
enum class RoomMode : int32_t {
    Off = 0,
    Room_15dB,  // subtle, safe for all material
    Room_14dB,  // recommended for most music
    Room_13dB,  // light, good for slower tracks
    Room_12dB,  // moderate, may color transients
    Room_9dB,   // strong, for ambient/pad-heavy music
    Count
};

class RoomSim {
public:
    void setup(double sampleRate, RoomMode mode) {
        m_mode = mode;
        // 3ms early-reflection delay
        m_delaySamples = static_cast<int>(sampleRate * 0.003 + 0.5);
        if (m_delaySamples >= MAX_DELAY)
            m_delaySamples = MAX_DELAY - 1;
        if (m_delaySamples < 1)
            m_delaySamples = 1;

        // One-pole LP coefficient ~10kHz: reference 0.7 @48kHz
        // coef = 1 - exp(-2*pi*fc/rate), fc such that 0.7 @48kHz => ~9.2kHz
        const double fc = 9200.0;
        m_lpCoef = static_cast<float>(1.0 - std::exp(-2.0 * 3.14159265358979323846 * fc / sampleRate));

        switch (mode) {
            case RoomMode::Room_15dB: m_level = 0.178f; break;
            case RoomMode::Room_14dB: m_level = 0.20f; break;
            case RoomMode::Room_13dB: m_level = 0.224f; break;
            case RoomMode::Room_12dB: m_level = 0.25f; break;
            case RoomMode::Room_9dB:  m_level = 0.35f; break;
            default:                  m_level = 0.0f; break;
        }
        reset();
    }

    void reset() {
        m_delayL.fill(0.0f);
        m_delayR.fill(0.0f);
        m_delayIdx = 0;
        m_lpL = m_lpR = 0.0f;
    }

    bool enabled() const { return m_mode != RoomMode::Off; }

    // In-place processing of one stereo sample pair.
    void process(float& l, float& r) {
        if (m_mode == RoomMode::Off)
            return;

        m_delayL[m_delayIdx] = l;
        m_delayR[m_delayIdx] = r;

        int delayedIdx = m_delayIdx - m_delaySamples;
        if (delayedIdx < 0)
            delayedIdx += MAX_DELAY;
        const float delayedL = m_delayR[delayedIdx];  // R->L
        const float delayedR = m_delayL[delayedIdx];  // L->R

        // Gentle lowpass (~10kHz - air absorption, not head shadow)
        m_lpL += m_lpCoef * (delayedL - m_lpL);
        m_lpR += m_lpCoef * (delayedR - m_lpR);

        l += m_lpL * m_level;
        r += m_lpR * m_level;

        m_delayIdx++;
        if (m_delayIdx >= MAX_DELAY)
            m_delayIdx = 0;
    }

private:
    static constexpr int MAX_DELAY = 1024;  // ~21ms @48kHz, plenty for 3ms
    std::array<float, MAX_DELAY> m_delayL{};
    std::array<float, MAX_DELAY> m_delayR{};
    int m_delayIdx = 0;
    int m_delaySamples = 144;  // 3ms @ 48kHz
    float m_lpL = 0.0f, m_lpR = 0.0f;
    float m_lpCoef = 0.7f;
    float m_level = 0.0f;
    RoomMode m_mode = RoomMode::Off;
};

// Parses a room mode config string: "off", "-15db", "-14db", "-13db",
// "-12db", "-9db" (leading '-' and "db" suffix optional, case-insensitive).
// Unknown strings map to Off.
inline RoomMode roomModeFromString(const char* s) {
    if (!s || !*s)
        return RoomMode::Off;
    if (*s == '-')
        s++;
    int v = 0;
    while (*s >= '0' && *s <= '9')
        v = v * 10 + (*s++ - '0');
    switch (v) {
        case 15: return RoomMode::Room_15dB;
        case 14: return RoomMode::Room_14dB;
        case 13: return RoomMode::Room_13dB;
        case 12: return RoomMode::Room_12dB;
        case 9:  return RoomMode::Room_9dB;
        default: return RoomMode::Off;
    }
}

// Convenience wrapper: full PWM-engine post chain (punch + room).
class PwmPostChain {
public:
    void setup(double sampleRate, bool punch, RoomMode room) {
        m_punchEnabled = punch;
        m_punch.setup(sampleRate);
        m_room.setup(sampleRate, room);
    }

    void reset() {
        m_punch.reset();
        m_room.reset();
    }

    void process(float& l, float& r) {
        if (m_punchEnabled)
            m_punch.process(l, r);
        m_room.process(l, r);
    }

    bool active() const { return m_punchEnabled || m_room.enabled(); }

private:
    PunchEnhancer m_punch;
    RoomSim m_room;
    bool m_punchEnabled = false;
};

}  // namespace qsr_dsp
