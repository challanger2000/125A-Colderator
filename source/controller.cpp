#include "controller.h"
#include "parameters.h"
#include "state_format.h"

#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/vstparameters.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace Colderator {

tresult PLUGIN_API Controller::initialize(FUnknown* context)
{
    auto r = EditController::initialize(context);
    if (r != kResultOk)
        return r;

    auto addPercent = [&](const TChar* title, ParamID id) {
        auto* p = new RangeParameter(title, id, STR16("%"), 0.0, 100.0, 0.0);
        p->setPrecision(0);
        parameters.addParameter(p);
    };

    addPercent(STR16("Cold"), kCold);
    addPercent(STR16("Ice"), kIce);
    addPercent(STR16("Metal"), kMetal);
    addPercent(STR16("Frost"), kFrost);
    addPercent(STR16("Shiver"), kShiver);
    addPercent(STR16("Space"), kSpace);

    auto* output = new RangeParameter(STR16("Output"), kOutput, STR16("dB"), -12.0, 12.0, 0.0);
    output->setPrecision(1);
    parameters.addParameter(output);

    auto* bypass = new StringListParameter(
        STR16("Bypass"),
        kBypass,
        nullptr,
        ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass | ParameterInfo::kIsList);
    bypass->appendString(STR16("Off"));
    bypass->appendString(STR16("On"));
    parameters.addParameter(bypass);

    return kResultOk;
}

tresult PLUGIN_API Controller::setComponentState(IBStream* state)
{
    if (!state)
        return kResultFalse;

    IBStreamer stream(state, kLittleEndian);
    float values[kComponentStateValueCount] {};
    int32 bypass = 0;

    if (!readComponentStatePayload(stream, values, bypass))
        return kResultFalse;

    const ParamID ids[kComponentStateValueCount] = {
        kCold, kIce, kMetal, kFrost, kShiver, kSpace, kOutput
    };

    for (int i = 0; i < kComponentStateValueCount; ++i)
        setParamNormalized(ids[i], values[i]);

    setParamNormalized(kBypass, bypass ? 1.0 : 0.0);
    return kResultOk;
}

} // namespace Colderator
