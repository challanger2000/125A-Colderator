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
    kBypass,

    // Stable material selectors. Keep existing IDs untouched.
    kIceMaterial = 108,
    kMetalMaterial,
    kFrostMaterial,
    kShiverMaterial,
    kSpaceMaterial,

    // Cinematic atmosphere slots.
    kAtmosAType = 113,
    kAtmosAAmount,
    kAtmosBType,
    kAtmosBAmount
};

constexpr int kMaterialCount = 6;
constexpr int kAtmosphereTypeCount = 10;

} // namespace Colderator
