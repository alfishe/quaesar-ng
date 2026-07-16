// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "Infrastructure/Reflection.h"

//
// Enumerations
//

namespace vamiga {

enum class SamplingMethod
{
    NONE,
    NEAREST,
    LINEAR,

    // Quaesar extension: boxcar (first-order CIC) averaging of the Paula
    // staircase over each output sample window. Emulates the "PWM engine"
    // decimation character (gentle ~-3.9dB rolloff at Nyquist).
    PWM
};

struct SamplingMethodEnum : Reflection<SamplingMethodEnum, SamplingMethod>
{
    static constexpr long minVal = 0;
    static constexpr long maxVal = long(SamplingMethod::PWM);

    static const char *_key(SamplingMethod value)
    {
        switch (value) {

            case SamplingMethod::NONE:     return "NONE";
            case SamplingMethod::NEAREST:  return "NEAREST";
            case SamplingMethod::LINEAR:   return "LINEAR";
            case SamplingMethod::PWM:      return "PWM";
        }
        return "???";
    }
    static const char *help(SamplingMethod value)
    {
        switch (value) {

            case SamplingMethod::NONE:     return "Latest sample";
            case SamplingMethod::NEAREST:  return "Nearest neighbor";
            case SamplingMethod::LINEAR:   return "Linear interpolation";
            case SamplingMethod::PWM:      return "Boxcar (CIC) averaging";
        }
        return "???";
    }
};

}
