// Majority of the code has been taken from https://github.com/BlitterStudio/amiberry/blob/master/src/sounddep/sound.cpp
// and is written by Dimitris Panokostas
// and licensed under GPL 2.0

// clang-format off
#include <string.h>
//#include <unistd.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include <math.h>

#include "options.h"
#include "audio.h"
#include "memory.h"
#include "events.h"
#include "custom.h"
#include "threaddep/thread.h"
#include "gui.h"
#include "savestate.h"
#ifdef DRIVESOUND
#include "driveclick.h"
#endif
#include "gensound.h"
#include "xwin.h"
#include "sounddep/sound.h"
#include <SDL.h>
// clang-format on

#define AMIBERRY
#define SOUND_DEVICE_SDL2 7

// #include "cda_play.h"

struct sound_dp {
    SDL_AudioDeviceID dev;
    int sndbufsize;
    int framesperbuffer;
    int sndbuf;
    int pullmode;
    uae_u8* pullbuffer;
    unsigned int pullbufferlen;
    int pullbuffermaxlen;
    float avg_correct;
    float cnt_correct;
    int stream_initialised;
    int silence_written;
};

struct sound_device {
    int id;
    TCHAR* name;
    TCHAR* alname;
    TCHAR* cfgname;
    TCHAR* prefix;
    int panum;
    int type;
};

#define SND_STATUSCNT 10

#define ADJUST_SIZE 20
#define EXP 1.9

#define ADJUST_VSSIZE 12
#define EXPVS 1.6

static int have_sound = 0;
static int statuscnt;

#define SND_MAX_BUFFER2 524288
#define SND_MAX_BUFFER 65536

uae_u16 paula_sndbuffer[SND_MAX_BUFFER];
uae_u16* paula_sndbufpt;
int paula_sndbufsize;
int active_sound_stereo;

#ifdef AMIBERRY
void sdl2_audio_callback(void* userdata, Uint8* stream, int len);
#endif

#define MAX_SOUND_DEVICES 100
struct sound_device* sound_devices[MAX_SOUND_DEVICES];
struct sound_device* record_devices[MAX_SOUND_DEVICES];
static int num_sound_devices, num_record_devices;

static struct sound_data sdpaula;
static struct sound_data* sdp = &sdpaula;

static uae_u8* extrasndbuf;
static int extrasndbufsize;
static int extrasndbuffered;

// Semaphore for audio-callback-driven frame synchronization.
// The SDL audio callback posts this once per frame's worth of audio consumed,
// signaling the emulation thread to compute the next frame.
// This implements the same crystal-locked timing pattern as unreal-ng's
// MainLoop (see NC_AUDIO_BUFFER_HALF_FULL).
static SDL_sem* s_frame_sync_sem = NULL;
static unsigned int s_consumed_since_post = 0;  // bytes consumed since last semaphore post

int setup_sound(void) {
    sound_available = 1;
    return 1;
}

float sound_sync_multiplier = 1.0;
float scaled_sample_evtime_orig;
extern float sampler_evtime;

void update_sound(float clk) {
    if (!have_sound)
        return;
    scaled_sample_evtime_orig = clk * (float)CYCLE_UNIT * sound_sync_multiplier / static_cast<float>(sdp->obtainedfreq);
    scaled_sample_evtime = scaled_sample_evtime_orig;
    // sampler_evtime = clk * CYCLE_UNIT * sound_sync_multiplier;
}

extern frame_time_t vsynctimebase_orig;

#define ADJUST_LIMIT 6
#define ADJUST_LIMIT2 1

void sound_setadjust(float v) {
    if (v < -ADJUST_LIMIT)
        v = -ADJUST_LIMIT;
    if (v > ADJUST_LIMIT)
        v = ADJUST_LIMIT;

    const float mult = 1000.0f + v;
    if (isvsync_chipset()) {
        vsynctimebase = vsynctimebase_orig;
        scaled_sample_evtime = scaled_sample_evtime_orig * mult / 1000.0f;
    } else if (currprefs.cachesize || currprefs.m68k_speed != 0) {
        vsynctimebase = static_cast<int>(static_cast<double>(vsynctimebase_orig) * mult / 1000.0f);
        scaled_sample_evtime = scaled_sample_evtime_orig;
    } else {
        vsynctimebase = static_cast<int>(static_cast<double>(vsynctimebase_orig) * mult / 1000.0f);
        scaled_sample_evtime = scaled_sample_evtime_orig;
    }
}

#if 0  // UNUSED FUNCS
static void docorrection(struct sound_dp* s, int sndbuf, float sync, int granulaty) {
    static int tfprev;

    s->avg_correct += sync;
    s->cnt_correct++;

    if (granulaty < 10)
        granulaty = 10;

    if (tfprev != timeframes) {
        const int avg = s->avg_correct / s->cnt_correct;

        float skipmode = sync / 100.0f;
        const float avgskipmode = avg / (10000.0f / granulaty);

        gui_data.sndbuf = sndbuf;

        if (skipmode > ADJUST_LIMIT2)
            skipmode = ADJUST_LIMIT2;
        if (skipmode < -ADJUST_LIMIT2)
            skipmode = -ADJUST_LIMIT2;

        sound_setadjust(skipmode + avgskipmode);
        tfprev = static_cast<int>(timeframes);
    }
}

static float sync_sound(float m) {
    float skipmode;
    if (isvsync()) {
        skipmode = (float)pow(m < 0 ? -m : m, EXPVS) / 2.0f;
        if (m < 0)
            skipmode = -skipmode;
        if (skipmode < -ADJUST_VSSIZE)
            skipmode = -ADJUST_VSSIZE;
        if (skipmode > ADJUST_VSSIZE)
            skipmode = ADJUST_VSSIZE;

    } else {
        skipmode = (float)pow(m < 0 ? -m : m, EXP) / 2.0f;
        if (m < 0)
            skipmode = -skipmode;
        if (skipmode < -ADJUST_SIZE)
            skipmode = -ADJUST_SIZE;
        if (skipmode > ADJUST_SIZE)
            skipmode = ADJUST_SIZE;
    }

    return skipmode;
}

static void set_reset(struct sound_data* sd) {
    sd->reset = true;
    sd->resetcnt = 10;
    sd->resetframecnt = 0;
}

static void disable_sound() {
    close_sound();
    currprefs.produce_sound = changed_prefs.produce_sound = 1;
}

#endif  // 0

static void clearbuffer_sdl2(struct sound_data* sd) {
    const sound_dp* s = sd->data;

    SDL_LockAudioDevice(s->dev);
    memset(paula_sndbuffer, 0, sizeof paula_sndbuffer);
    // Flush any queued audio data in push mode to prevent stale
    // samples playing after a reset.
    SDL_ClearQueuedAudio(s->dev);
    SDL_UnlockAudioDevice(s->dev);
}

static void clearbuffer(struct sound_data* sd) {
    sound_dp* s = sd->data;
    if (sd->devicetype == SOUND_DEVICE_SDL2)
        clearbuffer_sdl2(sd);
    if (s->pullbuffer) {
        memset(s->pullbuffer, 0, s->pullbuffermaxlen);
    }
}


static void pause_audio_sdl2(struct sound_data* sd) {
    const sound_dp* s = sd->data;

    sd->waiting_for_buffer = 0;
    SDL_PauseAudioDevice(s->dev, 1);
    clearbuffer(sd);
}
static void resume_audio_sdl2(struct sound_data* sd) {
    sound_dp* s = sd->data;

    clearbuffer(sd);

    // Pre-fill pull buffer with ~3 frames of silence to build an initial
    // reserve. Since emulation production rate matches audio consumption rate
    // (crystal-locked), this margin persists indefinitely and absorbs
    // occasional over-budget frames without starving the audio callback.
    // 3 PAL frames * 3528 bytes/frame ≈ 10.5ms latency — imperceptible.
    if (s->pullmode && s->pullbuffer) {
        extern float fake_vblank_hz;
        float hz = fake_vblank_hz > 0.1f ? fake_vblank_hz : 50.0f;
        unsigned int bytes_per_frame = (unsigned int)(sd->freq * sd->samplesize / hz);
        unsigned int prefill = bytes_per_frame * 3;
        if (prefill > s->pullbuffermaxlen)
            prefill = s->pullbuffermaxlen / 2;
        s->pullbufferlen = prefill;
        // Buffer is already zeroed by clearbuffer, so silence is pre-filled
        s_consumed_since_post = 0;
    }

    sd->waiting_for_buffer = 1;
    s->avg_correct = 0;
    s->cnt_correct = 0;
    SDL_PauseAudioDevice(s->dev, 0);
    sd->paused = 0;
}

static void close_audio_sdl2(struct sound_data* sd) {
    sound_dp* s = sd->data;
    SDL_PauseAudioDevice(s->dev, 1);

    SDL_LockAudioDevice(s->dev);
    if (s->pullbuffer != nullptr) {
        xfree(s->pullbuffer);
        s->pullbuffer = nullptr;
    }
    s->pullbufferlen = 0;
    SDL_UnlockAudioDevice(s->dev);

    SDL_CloseAudioDevice(s->dev);
}

// extern void setvolume_ahi(int);

void set_volume_sound_device(struct sound_data* sd, int volume, int mute) {
    // sound_dp* s = sd->data;
    if (sd->devicetype == SOUND_DEVICE_SDL2) {
        if (volume < 100 && !mute)
            volume = 100 - volume;
        else if (mute || volume >= 100)
            volume = 0;
        // TODO switch to using SDL_mixer to implement volume control properly
        // SDL_MixAudioFormat(reinterpret_cast<uae_u8*>(s->sndbuf), reinterpret_cast<uae_u8*>(s->sndbuf), AUDIO_S16,
        // sd->sndbufsize, volume);
    }
}

void set_volume(int volume, int mute) {
    set_volume_sound_device(sdp, volume, mute);
    // setvolume_ahi(volume);
    config_changed = 1;
}

static void finish_sound_buffer_pull(struct sound_data* sd, uae_u16* sndbuffer) {
    sound_dp* s = sd->data;

    static int s_push_log_throttle = 0;
    static int s_overflow_cnt = 0;
    extern uae_u32 timeframes;

    if (s->pullbufferlen + sd->sndbufsize > s->pullbuffermaxlen) {
        s_overflow_cnt++;
        if (s_push_log_throttle <= 0) {
            SDL_Log(
                "AUDIO PULL OVERFLOW: cur=%u max=%u push=%d (count: %d, frame: %lu) - emulation producing faster than "
                "playback",
                s->pullbufferlen, s->pullbuffermaxlen, sd->sndbufsize, s_overflow_cnt, (unsigned long)timeframes);
            s_push_log_throttle = 50;
        }
        s->pullbufferlen = 0;
        gui_data.sndbuf_status = 1;
    } else {
        gui_data.sndbuf_status = 0;
    }
    if (s_push_log_throttle > 0)
        s_push_log_throttle--;

    memcpy(s->pullbuffer + s->pullbufferlen, sndbuffer, sd->sndbufsize);
    s->pullbufferlen += sd->sndbufsize;

    gui_data.sndbuf = (int)(1000.0f * s->pullbufferlen) / s->pullbuffermaxlen;
}

static int open_audio_sdl2(struct sound_data* sd, int index) {
    sound_dp* const s = sd->data;
    const int freq = sd->freq;
    const int ch = sd->channels;
    TCHAR* devname = sound_devices[index]->name;

    sd->devicetype = SOUND_DEVICE_SDL2;
    if (sd->sndbufsize < 0x80)
        sd->sndbufsize = 0x80;
    s->framesperbuffer = sd->sndbufsize;
    s->sndbufsize = s->framesperbuffer;
    sd->sndbufsize = s->sndbufsize * ch * 2;
    if (sd->sndbufsize > SND_MAX_BUFFER)
        sd->sndbufsize = SND_MAX_BUFFER;
    sd->samplesize = ch * 16 / 8;
    // Use pull/callback mode for crystal-locked frame synchronization.
    // The SDL audio callback runs on the audio thread at the sound card's
    // crystal rate and signals the emulation thread when the buffer needs
    // more data, providing sub-frame-precision timing.
    s->pullmode = 1;

    SDL_AudioSpec want = {}, have = {};
    want.freq = freq;
    want.format = AUDIO_S16SYS;
    want.channels = (uint8_t)ch;

    if (s->pullmode) {
        // Small device buffer for frequent callbacks (~86 Hz at 44100 Hz).
        // This gives fine-grained frame-sync signaling: the callback fires
        // roughly every 0.58 PAL frames, enabling precise audio-locked pacing.
        want.samples = 512;
        want.callback = sdl2_audio_callback;
        want.userdata = sd;
    } else {
        // Push mode (fallback): 3x device buffer for jitter resilience.
        want.samples = (uint16_t)(s->framesperbuffer * 3);
    }

    // Allow SDL to adjust samples if the hardware requires a different size
    const int allow_change = s->pullmode ? SDL_AUDIO_ALLOW_SAMPLES_CHANGE : 0;
    if (s->dev == 0)
        s->dev = SDL_OpenAudioDevice(devname, 0, &want, &have, allow_change);
    if (s->dev == 0) {
        write_log("Failed to open selected SDL2 device for audio: %s, retrying with default device\n", SDL_GetError());
        s->dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, allow_change);
        if (s->dev == 0) {
            write_log("Failed to open default SDL2 device for audio: %s\n", SDL_GetError());
            return 0;
        }
    }

    // Create (or reset) the frame-sync semaphore for callback-driven timing
    if (s->pullmode) {
        if (s_frame_sync_sem == NULL)
            s_frame_sync_sem = SDL_CreateSemaphore(0);
        else
            while (SDL_SemTryWait(s_frame_sync_sem) == 0) {
            }  // drain stale posts
        s_consumed_since_post = 0;  // reset rate limiter
    }

    if (s->pullmode) {
        // Generous pull buffer: 8x Paula buffer to absorb timing jitter
        // between per-frame Paula pushes and smooth callback draining.
        s->pullbuffermaxlen = sd->sndbufsize * 8;
        s->pullbuffer = xcalloc(uae_u8, s->pullbuffermaxlen);
        s->pullbufferlen = 0;
    }
    write_log("SDL2: CH=%d, FREQ=%d '%s' buffer %d/%d device_samples=%d (%s)\n", ch, freq, sound_devices[index]->name,
              s->sndbufsize, s->framesperbuffer, have.samples, !s->pullmode ? _T("push") : _T("pull"));
    clearbuffer(sd);

    return 1;
}

int open_sound_device(struct sound_data* sd, int index, int bufsize, int freq, int channels) {
    sound_dp* dp = xcalloc(struct sound_dp, 1);

    sd->data = dp;
    sd->sndbufsize = bufsize;
    sd->freq = freq;
    sd->channels = channels;
    sd->paused = 1;
    sd->index = index;
    const int ret = open_audio_sdl2(sd, index);
    sd->samplesize = sd->channels * 2;
    sd->sndbufframes = sd->sndbufsize / sd->samplesize;
    return ret;
}

void close_sound_device(struct sound_data* sd) {
    pause_sound_device(sd);
    close_audio_sdl2(sd);
    xfree(sd->data);
    sd->data = NULL;
    sd->index = -1;
}

void pause_sound_device(struct sound_data* sd) {
    sd->paused = 1;
    gui_data.sndbuf_status = 0;
    gui_data.sndbuf = 0;
    pause_audio_sdl2(sd);
}
void resume_sound_device(struct sound_data* sd) {
    resume_audio_sdl2(sd);
    sd->paused = 0;
}

int get_default_audio_device() {
#if 0
    int device_idx = -1;
#if SDL_VERSION_ATLEAST(2, 24, 0)
    SDL_AudioSpec spec;
    char* default_device_name = nullptr;
    if (SDL_GetDefaultAudioInfo(&default_device_name, &spec, 0) == 0) {
        for (int i = 0; i < num_sound_devices; i++) {
            if (strcmp(sound_devices[i]->name, default_device_name) != 0)
                continue;
            device_idx = i;
            break;
        }
        SDL_free(default_device_name);
    }
#endif
    return device_idx;
#else
    return 0;  // The first device (index 0) is now 'System Default'
#endif
}

static int open_sound() {
    int size = currprefs.sound_maxbsiz;

    if (!currprefs.produce_sound)
        return 0;
    config_changed = 1;
    /* Always interpret buffer size as number of samples, not as actual
    buffer size.  Of course, since 8192 is the default, we'll have to
    scale that to a sane value (assuming that otherwise 16 bits and
    stereo would have been enabled and we'd have done the shift by
    two anyway).  */
    size >>= 2;
    size &= ~63;

    // Pull/callback mode: use a small Paula buffer (~1 PAL frame = 882 samples)
    // so audio is pushed to the pull buffer every frame, not in bursts every
    // ~4.6 frames. This is critical for smooth callback-driven frame sync —
    // bursty production causes the pull buffer to oscillate between full and
    // empty, producing regular choppiness.
    if (size > 1024)
        size = 1024;

    sdp->softvolume = -1;
    int num = enumerate_sound_devices();
    if (num == 0)
        return 0;
    if (currprefs.win32_soundcard < 0)
        currprefs.win32_soundcard = changed_prefs.win32_soundcard = get_default_audio_device();
    if ((unsigned)currprefs.win32_soundcard >= (unsigned)num)
        currprefs.win32_soundcard = changed_prefs.win32_soundcard = 0;
    const int ch = get_audio_nativechannels(active_sound_stereo);
    const int ret = open_sound_device(sdp, currprefs.win32_soundcard, size, currprefs.sound_freq, ch);
    if (!ret)
        return 0;
    currprefs.sound_freq = changed_prefs.sound_freq = sdp->freq;
    if (ch != sdp->channels)
        active_sound_stereo = get_audio_stereomode(sdp->channels);

    set_volume(currprefs.sound_volume_master, sdp->mute);
    if (get_audio_amigachannels(active_sound_stereo) == 4)
        sample_handler = sample16ss_handler;
    else
        sample_handler = get_audio_ismono(active_sound_stereo) ? sample16_handler : sample16s_handler;

    sdp->obtainedfreq = currprefs.sound_freq;

    have_sound = 1;
    sound_available = 1;
#ifdef AMIBERRY
    // Always show sound buffer usage
    gui_data.sndbuf_avail = true;
#else
    gui_data.sndbuf_avail = audio_is_pull() == 0;
#endif

    paula_sndbufsize = sdp->sndbufsize;
    paula_sndbufpt = paula_sndbuffer;
#ifdef DRIVESOUND
    driveclick_init();
#endif
    return 1;
}

void close_sound() {
    config_changed = 1;
    gui_data.sndbuf = 0;
    gui_data.sndbuf_status = 3;
    gui_data.sndbuf_avail = false;
    if (!have_sound)
        return;
    close_sound_device(sdp);
    have_sound = 0;
    extrasndbufsize = 0;
    extrasndbuffered = 0;
    xfree(extrasndbuf);
    extrasndbuf = nullptr;
}

bool sound_paused() {
    return sdp->paused != 0;
}

void pause_sound() {
    if (sdp->paused)
        return;
    if (!have_sound)
        return;
    pause_sound_device(sdp);
}

void resume_sound() {
    if (!sdp->paused)
        return;
    if (!have_sound)
        return;
    resume_sound_device(sdp);
}

void reset_sound() {
    if (!have_sound)
        return;
    clearbuffer(sdp);
}

void dummy_sound() {
}

int init_sound() {
    // bool started = false;
    gui_data.sndbuf_status = 3;
    gui_data.sndbuf = 0;
    gui_data.sndbuf_avail = false;
    sample_handler = dummy_sound;
    if (!sound_available)
        return 0;
    if (currprefs.produce_sound <= 1)
        return 0;
    if (have_sound)
        return 1;
    if (!open_sound())
        return 0;
    sdp->paused = 1;
#ifdef DRIVESOUND
    driveclick_reset();
#endif
    reset_sound();
    resume_sound();
    /*
    if (!started &&
        (currprefs.start_minimized && currprefs.minimized_nosound ||
            currprefs.start_uncaptured && currprefs.inactive_nosound))
        pause_sound();
    started = true;
    */
    return 1;
}

#if 0
static void disable_sound() {
    close_sound();
    currprefs.produce_sound = changed_prefs.produce_sound = 1;
}
#endif  //

static int reopen_sound(void) {
    const bool paused = sdp->paused != 0;
    close_sound();
    const int v = open_sound();
    if (v && !paused)
        resume_sound_device(sdp);
    return v;
}

void pause_sound_buffer() {
    sdp->deactive = true;
    reset_sound();
}

void restart_sound_buffer() {
    sdp->deactive = false;
    // restart_sound_buffer2(sdp);
}


// Audio-callback-driven frame sync (pull/callback mode).
// Waits on the semaphore posted by sdl2_audio_callback when the pull buffer
// drops below 50%. This locks emulation frame timing to the sound card
// crystal oscillator, providing sub-frame precision.
// max_wait_ms: maximum time to block in milliseconds.
//   20 = one PAL frame (standard pacing timeout).
// Returns: 0 = proceed with frame, -1 = fallback to wall clock.
int audio_callback_sync_wait_ms(int max_wait_ms) {
    if (!have_sound || sdp->deactive || sdp->paused || sdp->reset)
        return -1;
    sound_dp* s = sdp->data;
    if (!s || !s->pullmode || s->dev == 0)
        return -1;
    if (!s_frame_sync_sem)
        return -1;

    // Wait for the audio callback to signal that one frame's worth of audio
    // has been consumed by the sound card crystal.
    int timeout_ms = max_wait_ms;
    if (timeout_ms < 0 || timeout_ms > 20)
        timeout_ms = 20;
    SDL_SemWaitTimeout(s_frame_sync_sem, timeout_ms);

    // Drain any additional posts to prevent semaphore value buildup.
    while (SDL_SemTryWait(s_frame_sync_sem) == 0) {
    }

    return 0;  // proceed with next frame
}

static void finish_sound_buffer_sdl2_push(struct sound_data* sd, uae_u16* sndbuffer) {
    sound_dp* s = sd->data;
    if (sd->mute) {
        memset(sndbuffer, 0, sd->sndbufsize);
        s->silence_written++;
    }

    SDL_QueueAudio(s->dev, sndbuffer, sd->sndbufsize);
}

static void finish_sound_buffer_sdl2(struct sound_data* sd, uae_u16* sndbuffer) {
    const sound_dp* s = sd->data;
    if (!sd->waiting_for_buffer)
        return;

    if (s->pullmode)
        finish_sound_buffer_pull(sd, sndbuffer);
    else
        finish_sound_buffer_sdl2_push(sd, sndbuffer);
}

static void channelswap(uae_s16* sndbuffer, int len) {
    for (int i = 0; i < len; i += 2) {
        const uae_s16 t = sndbuffer[i];
        sndbuffer[i] = sndbuffer[i + 1];
        sndbuffer[i + 1] = t;
    }
}
static void channelswap6(uae_s16* sndbuffer, int len) {
    for (int i = 0; i < len; i += 6) {
        uae_s16 t = sndbuffer[i + 0];
        sndbuffer[i + 0] = sndbuffer[i + 1];
        sndbuffer[i + 1] = t;
        t = sndbuffer[i + 4];
        sndbuffer[i + 4] = sndbuffer[i + 5];
        sndbuffer[i + 5] = t;
    }
}

static bool send_sound_do(struct sound_data* sd) {
    const int type = sd->devicetype;
    if (type == SOUND_DEVICE_SDL2) {
        finish_sound_buffer_pull(sd, paula_sndbuffer);
        return true;
    }
    return false;
}

static void send_sound(struct sound_data* sd, uae_u16* sndbuffer) {
    const int type = sd->devicetype;
    if (savestate_state)
        return;
    if (sd->paused)
        return;
    if (sd->softvolume >= 0) {
        uae_s16* p = reinterpret_cast<uae_s16*>(sndbuffer);
        for (int i = 0; i < sd->sndbufsize / 2; i++) {
            p[i] = uae_s16((p[i] * sd->softvolume) / 32768);
        }
    }
    if (type == SOUND_DEVICE_SDL2)
        finish_sound_buffer_sdl2(sd, sndbuffer);
}

int get_sound_event(void) {
    // int type = sdp->devicetype;
    if (sdp->paused || sdp->deactive)
        return 0;
    // if (type == SOUND_DEVICE_WASAPI || type == SOUND_DEVICE_WASAPI_EXCLUSIVE || type == SOUND_DEVICE_PA) {
    //	struct sound_dp* s = sdp->data;
    //	if (s && s->pullmode) {
    //		return s->pullevent;
    //	}
    // }
    return 0;
}

bool audio_is_event_frame_possible(int) {
    const int type = sdp->devicetype;
    if (sdp->paused || sdp->deactive || sdp->reset)
        return false;
    if (type == SOUND_DEVICE_SDL2) {
        sound_dp* s = sdp->data;
        ptrdiff_t bufsize = reinterpret_cast<uae_u8*>(paula_sndbufpt) - reinterpret_cast<uae_u8*>(paula_sndbuffer);
        bufsize /= sdp->samplesize;
        const int todo = s->sndbufsize - (int)bufsize;
        int samplesperframe = sdp->obtainedfreq / static_cast<int>(vblank_hz);
        return samplesperframe >= todo - samplesperframe;
    }
    return false;
}

int audio_is_pull() {
    if (sdp->reset)
        return 0;
    sound_dp* s = sdp->data;
    if (s && s->pullmode) {
        return sdp->paused || sdp->deactive ? -1 : 1;
    }
    return 0;
}

int audio_pull_buffer() {
    int cnt = 0;
    if (sdp->paused || sdp->deactive || sdp->reset)
        return 0;
    const struct sound_dp* s = sdp->data;
    if (s->pullbufferlen > 0) {
        cnt++;
        const size_t size = reinterpret_cast<uae_u8*>(paula_sndbufpt) - reinterpret_cast<uae_u8*>(paula_sndbuffer);
        if (size > static_cast<size_t>(sdp->sndbufsize) * 2 / 3)
            cnt++;
    }
    return cnt;
}

bool audio_is_pull_event() {
    if (sdp->paused || sdp->deactive || sdp->reset)
        return false;
    return false;
}

bool audio_finish_pull() {
    const int type = sdp->devicetype;
    if (sdp->paused || sdp->deactive || sdp->reset)
        return false;
    if (type != SOUND_DEVICE_SDL2)
        return false;
    if (audio_pull_buffer() && audio_is_pull_event()) {
        return send_sound_do(sdp);
    }
    return false;
}

static void handle_reset() {
    if ((uint32_t)sdp->resetframe == timeframes)
        return;
    sdp->resetframe = static_cast<int>(timeframes);
    sdp->resetframecnt--;
    if (sdp->resetframecnt > 0)
        return;
    sdp->resetframecnt = 20;

    sdp->reset = false;
    if (!reopen_sound() || sdp->reset) {
        if (sdp->resetcnt <= 0) {
            write_log(_T("Reopen sound failed. Retrying with default device.\n"));
            close_sound();
            int type = sound_devices[currprefs.win32_soundcard]->type;
            int max = enumerate_sound_devices();
            for (int i = 0; i < max; i++) {
                if (sound_devices[i]->alname == NULL && sound_devices[i]->type == type) {
                    currprefs.win32_soundcard = changed_prefs.win32_soundcard = i;
                    if (open_sound())
                        return;
                    break;
                }
            }
            currprefs.produce_sound = changed_prefs.produce_sound = 1;
        } else {
            write_log(_T("Retrying sound.. %d..\n"), sdp->resetcnt);
            sdp->resetcnt--;
            sdp->reset = true;
        }
    } else {
        resume_sound_device(sdp);
    }
}

void finish_sound_buffer() {
    static unsigned long tframe;
    int bufsize = (int)(reinterpret_cast<uae_u8*>(paula_sndbufpt) - reinterpret_cast<uae_u8*>(paula_sndbuffer));

    if (sdp->reset) {
        handle_reset();
        paula_sndbufpt = paula_sndbuffer;
        return;
    }

    if (currprefs.turbo_emulation) {
        paula_sndbufpt = paula_sndbuffer;
        return;
    }
    if (currprefs.sound_stereo_swap_paula) {
        if (get_audio_nativechannels(active_sound_stereo) == 2 || get_audio_nativechannels(active_sound_stereo) == 4)
            channelswap(reinterpret_cast<uae_s16*>(paula_sndbuffer), bufsize / 2);
        else if (get_audio_nativechannels(active_sound_stereo) >= 6)
            channelswap6(reinterpret_cast<uae_s16*>(paula_sndbuffer), bufsize / 2);
    }
#ifdef DRIVESOUND
    driveclick_mix(reinterpret_cast<uae_s16*>(paula_sndbuffer), bufsize / 2, currprefs.dfxclickchannelmask);
#endif
    // must be after driveclick_mix
    paula_sndbufpt = paula_sndbuffer;

    if (!have_sound)
        return;

    // we got buffer that was not full (recording active). Need special handling.
    if (bufsize < sdp->sndbufsize && !extrasndbuf) {
        extrasndbufsize = sdp->sndbufsize;
        extrasndbuf = xcalloc(uae_u8, sdp->sndbufsize);
        extrasndbuffered = 0;
    }

    if (statuscnt > 0 && tframe != timeframes) {
        tframe = timeframes;
        statuscnt--;
        if (statuscnt == 0)
            gui_data.sndbuf_status = 0;
    }
    if (gui_data.sndbuf_status == 3)
        gui_data.sndbuf_status = 0;

    if (extrasndbuf) {
        size_t size = extrasndbuffered + bufsize;
        size_t copied = 0;
        if (size > extrasndbufsize) {
            copied = extrasndbufsize - extrasndbuffered;
            memcpy(extrasndbuf + extrasndbuffered, paula_sndbuffer, copied);
            send_sound(sdp, reinterpret_cast<uae_u16*>(extrasndbuf));
            extrasndbuffered = 0;
        }
        memcpy(extrasndbuf + extrasndbuffered, reinterpret_cast<uae_u8*>(paula_sndbuffer) + copied, bufsize - copied);
        extrasndbuffered += (int)(bufsize - copied);
    } else {
        send_sound(sdp, paula_sndbuffer);
    }
}

int enumerate_sound_devices() {
    if (!num_sound_devices) {
        write_log("Enumerating SDL2 playback devices...\n");
        int sdl_num = SDL_GetNumAudioDevices(SDL_FALSE);
        write_log("Detected %d sound playback devices\n", sdl_num);

        sound_devices[0] = xcalloc(struct sound_device, 1);
        sound_devices[0]->id = 0;
        sound_devices[0]->cfgname = my_strdup("System Default");
        sound_devices[0]->type = SOUND_DEVICE_SDL2;
        sound_devices[0]->name = nullptr;
        sound_devices[0]->alname = my_strdup("0");
        num_sound_devices = 1;

        for (int i = 0; i < sdl_num && num_sound_devices < MAX_SOUND_DEVICES; i++) {
            const char* devname = SDL_GetAudioDeviceName(i, SDL_FALSE);
            write_log("Sound playback device %d: %s\n", num_sound_devices, devname);
            sound_devices[num_sound_devices] = xcalloc(struct sound_device, 1);
            sound_devices[num_sound_devices]->id = num_sound_devices;
            sound_devices[num_sound_devices]->cfgname = my_strdup(devname);
            sound_devices[num_sound_devices]->type = SOUND_DEVICE_SDL2;
            sound_devices[num_sound_devices]->name = my_strdup(devname);
            sound_devices[num_sound_devices]->alname = my_strdup(std::to_string(num_sound_devices).c_str());
            num_sound_devices++;
        }

        write_log("Enumerating SDL2 recording devices...\n");
        int sdl_rec_num = SDL_GetNumAudioDevices(SDL_TRUE);
        write_log("Detected %d sound recording devices\n", sdl_rec_num);

        record_devices[0] = xcalloc(struct sound_device, 1);
        record_devices[0]->id = 0;
        record_devices[0]->cfgname = my_strdup("System Default");
        record_devices[0]->type = SOUND_DEVICE_SDL2;
        record_devices[0]->name = nullptr;
        record_devices[0]->alname = my_strdup("0");
        num_record_devices = 1;

        for (int i = 0; i < sdl_rec_num && num_record_devices < MAX_SOUND_DEVICES; i++) {
            const char* devname = SDL_GetAudioDeviceName(i, SDL_TRUE);
            write_log("Sound recording device %d: %s\n", num_record_devices, devname);
            record_devices[num_record_devices] = xcalloc(struct sound_device, 1);
            record_devices[num_record_devices]->id = num_record_devices;
            record_devices[num_record_devices]->cfgname = my_strdup(devname);
            record_devices[num_record_devices]->type = SOUND_DEVICE_SDL2;
            record_devices[num_record_devices]->name = my_strdup(devname);
            record_devices[num_record_devices]->alname = my_strdup(std::to_string(num_record_devices).c_str());
            num_record_devices++;
        }

        write_log(_T("Enumeration end\n"));
        for (num_sound_devices = 0; num_sound_devices < MAX_SOUND_DEVICES; num_sound_devices++) {
            if (sound_devices[num_sound_devices] == NULL)
                break;
        }
        for (num_record_devices = 0; num_record_devices < MAX_SOUND_DEVICES; num_record_devices++) {
            if (record_devices[num_record_devices] == NULL)
                break;
        }
    }
    return num_sound_devices;
}

static int set_master_volume(int volume, int mute) {
    set_volume(volume, mute);
    return 1;
}

static int get_master_volume(int* /*volume*/, int* /*mute*/) {
    return currprefs.sound_volume_master;
}

void sound_mute(int newmute) {
    if (newmute < 0)
        sdp->mute = sdp->mute ? 0 : 1;
    else
        sdp->mute = newmute;
    set_volume(currprefs.sound_volume_master, sdp->mute);
    config_changed = 1;
}

void sound_volume(int dir) {
    currprefs.sound_volume_master -= dir * 10;
    currprefs.sound_volume_cd -= dir * 10;
    if (currprefs.sound_volume_master < 0)
        currprefs.sound_volume_master = 0;
    if (currprefs.sound_volume_master > 100)
        currprefs.sound_volume_master = 100;
    changed_prefs.sound_volume_master = currprefs.sound_volume_master;
    if (currprefs.sound_volume_cd < 0)
        currprefs.sound_volume_cd = 0;
    if (currprefs.sound_volume_cd > 100)
        currprefs.sound_volume_cd = 100;
    changed_prefs.sound_volume_cd = currprefs.sound_volume_cd;
    set_volume(currprefs.sound_volume_master, sdp->mute);
    config_changed = 1;
}

void master_sound_volume(int dir) {
    int vol, mute;

    const int r = get_master_volume(&vol, &mute);
    if (!r)
        return;
    if (dir == 0)
        mute = mute ? 0 : 1;
    vol += dir * (65536 / 10);
    if (vol < 0)
        vol = 0;
    if (vol > SDL_MIX_MAXVOLUME)
        vol = SDL_MIX_MAXVOLUME;
    set_master_volume(vol, mute);
    config_changed = 1;
}

// Rate-limited frame sync: posts the semaphore once per frame's worth of
// audio consumed by the sound card. This converts the ~86 Hz callback rate
// into exactly 50 Hz (PAL) or 60 Hz (NTSC) frame-sync signals.

static void post_frame_sync_if_needed(const sound_data* sd) {
    // Bytes of audio consumed per emulated video frame
    extern float fake_vblank_hz;
    float hz = fake_vblank_hz > 0.1f ? fake_vblank_hz : 50.0f;
    unsigned int bytes_per_frame = (unsigned int)(sd->freq * sd->samplesize / hz);
    if (bytes_per_frame < 1)
        return;

    // Use while loop: if a single callback consumed multiple frames' worth
    // of audio (e.g., large device buffer), post multiple times to catch up.
    while (s_consumed_since_post >= bytes_per_frame) {
        s_consumed_since_post -= bytes_per_frame;
        if (s_frame_sync_sem)
            SDL_SemPost(s_frame_sync_sem);
    }
}

void sdl2_audio_callback(void* userdata, Uint8* stream, int len) {
    sound_data* sd = static_cast<sound_data*>(userdata);
    sound_dp* s = sd->data;

    if (!s->stream_initialised || sd->mute) {
        memset(stream, 0, len);
        if (sd->mute)
            s->silence_written++;
        s->stream_initialised = 1;
    }

    if (!s->framesperbuffer || sdp->deactive) {
        memset(stream, 0, len);
        return;
    }

    if (s->pullbufferlen <= 0) {
        // Underrun: no audio data available — output silence
        memset(stream, 0, len);
        gui_data.sndbuf_status = -1;
        // Still accumulate consumed bytes so the frame-sync rate stays correct
        s_consumed_since_post += (unsigned int)len;
        // Signal emulation if a full frame's worth of audio has been consumed
        post_frame_sync_if_needed(sd);
        return;
    }

    // Copy min(len, pullbufferlen) bytes from pull buffer to SDL stream
    unsigned int to_copy = (unsigned int)len;
    if (to_copy > s->pullbufferlen)
        to_copy = s->pullbufferlen;

    if (sd->mute == 0 && to_copy > 0)
        memcpy(stream, s->pullbuffer, to_copy);
    // Fill remainder with silence if pull buffer was shorter than requested
    if (to_copy < (unsigned int)len)
        memset(stream + to_copy, 0, len - to_copy);

    // Shift remaining data in pull buffer
    if (to_copy < s->pullbufferlen)
        memmove(s->pullbuffer, s->pullbuffer + to_copy, s->pullbufferlen - to_copy);
    s->pullbufferlen -= to_copy;

    // Update buffer status for UI
    gui_data.sndbuf = (int)(1000.0f * s->pullbufferlen) / s->pullbuffermaxlen;

    // Rate-limit semaphore posts to one per frame.
    // The callback fires at ~86 Hz but we need exactly 50 (PAL) or 60 (NTSC)
    // posts per second. Each post triggers one frame computation.
    s_consumed_since_post += to_copy;
    post_frame_sync_if_needed(sd);
}

int sound_get_silence() {
    const sound_dp* s = sdp->data;
    return s->silence_written;
}

void ahi_close_sound() {
    TRACE();
}

void x86_update_sound(float) {
    // UNIMPLEMENTED();
}

#if 0

#define SND_MAX_BUFFER 65536

uae_u16 paula_sndbuffer[SND_MAX_BUFFER];
uae_u16* paula_sndbufpt;

int paula_sndbufsize = 0;
int active_sound_stereo;

bool audio_finish_pull(void) {
    // UNIMPLEMENTED();
    return false;
}

void sound_volume(int) {
    UNIMPLEMENTED();
}

void set_volume(int, int) {
    UNIMPLEMENTED();
}

void master_sound_volume(int) {
    UNIMPLEMENTED();
}

void finish_sound_buffer() {
    printf("finish_sound_buffer\n");
}

int init_sound() {
    TRACE();
    return 0;
}

void pause_sound_buffer() {
    UNIMPLEMENTED();
}

void reset_sound() {
    TRACE();
}

void restart_sound_buffer() {
    TRACE();
}

int setup_sound() {
	sound_available = 1;
	return 1;
}

void sound_mute(int) {
    UNIMPLEMENTED();
}

void update_sound(float) {
    UNIMPLEMENTED();
}

void x86_update_sound(float) {
    UNIMPLEMENTED();
}

void close_sound() {
    UNIMPLEMENTED();
}

void pause_sound() {
    UNIMPLEMENTED();
}

void resume_sound() {
    TRACE();
    // UNIMPLEMENTED();
}

bool audio_is_pull_event() {
    // TRACE();
    return false;
}

int audio_pull_buffer() {
    UNIMPLEMENTED();
    return 0;
}

int audio_is_pull() {
    // TRACE();
    return 0;
}

#endif
