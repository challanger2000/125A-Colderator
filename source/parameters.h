#pragma once
#include "pluginterfaces/vst/vsttypes.h"

namespace Colderator {

enum ParamIds : Steinberg::Vst::ParamID
{
    kCold = 100,
    kIce,
    kMetal,
    kFrost,
    kShiver,
    kSpace,
    kOutput,
    kBypass
};

} // namespace Colderator
