#include "processor.h"
#include "controller.h"
#include "ids.h"
#include "parameters.h"
#include "state_format.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace Colderator {

namespace {
inline float clamp01(float v)
{
    return std::max(0.f, std::min(1.f, v));
}

inline float normalizedOutputToDb(float v)
{
    return -12.f + 24.f * clamp01(v);
}
}

Processor::Processor()
{
    setControllerClass(ControllerUID);
}

tresult PLUGIN_API Processor::initialize(FUnknown* context)
{
    auto r = AudioEffect::initialize(context);
    if (r != kResultOk)
        return r;

    addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
    return kResultOk;
}

tresult PLUGIN_API Processor::canProcessSampleSize(int32 symbolicSampleSize)
{
    return symbolicSampleSize == kSample32 ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API Processor::setBusArrangements(SpeakerArrangement* inputs, int32 numIns,
                                                  SpeakerArrangement* outputs, int32 numOuts)
{
    if (numIns == 1 && numOuts == 1 &&
        inputs[0] == SpeakerArr::kStereo && outputs[0] == SpeakerArr::kStereo)
        return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);

    return kResultFalse;
}

void Processor::applyParameter(ParamID id, float normalized)
{
    const float v = clamp01(normalized);

    switch (id)
    {
        case kCold:   cold_ = v; break;
        case kIce:    ice_ = v; break;
        case kMetal:  metal_ = v; break;
        case kFrost:  frost_ = v; break;
        case kShiver: shiver_ = v; break;
        case kSpace:  space_ = v; break;
        case kOutput: output_ = v; break;
        case kBypass: bypass_ = v >= 0.5f; break;
        default: break;
    }
}

tresult PLUGIN_API Processor::process(ProcessData& data)
{
    if (data.inputParameterChanges)
    {
        const int32 count = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < count; ++i)
        {
            if (auto* queue = data.inputParameterChanges->getParameterData(i))
            {
                const int32 points = queue->getPointCount();
                if (points > 0)
                {
                    int32 sampleOffset = 0;
                    ParamValue value = 0.0;
                    if (queue->getPoint(points - 1, sampleOffset, value) == kResultTrue)
                        applyParameter(queue->getParameterId(), static_cast<float>(value));
                }
            }
        }
    }

    if (data.numInputs < 1 || data.numOutputs < 1 || data.numSamples <= 0)
        return kResultOk;

    if (data.symbolicSampleSize != kSample32)
        return kResultFalse;

    const auto& inBus = data.inputs[0];
    auto& outBus = data.outputs[0];
    const int32 channels = std::min(inBus.numChannels, outBus.numChannels);

    const float gain = bypass_ ? 1.f : std::pow(10.f, normalizedOutputToDb(output_) / 20.f);

    for (int32 ch = 0; ch < channels; ++ch)
    {
        const float* in = inBus.channelBuffers32[ch];
        float* out = outBus.channelBuffers32[ch];
        if (!in || !out)
            continue;

        if (gain == 1.f)
        {
            if (in != out)
                std::memcpy(out, in, static_cast<size_t>(data.numSamples) * sizeof(float));
        }
        else
        {
            for (int32 s = 0; s < data.numSamples; ++s)
                out[s] = in[s] * gain;
        }
    }

    // v0.1.0 scaffold intentionally performs no COLD/ICE/METAL/FROST/SHIVER/SPACE DSP yet.
    // Each block will be introduced independently and measured against this neutral baseline.
    return kResultOk;
}

tresult PLUGIN_API Processor::setState(IBStream* state)
{
    if (!state)
        return kResultFalse;

    IBStreamer stream(state, kLittleEndian);
    float values[kComponentStateValueCount] {};
    int32 bypass = 0;

    if (!readComponentStatePayload(stream, values, bypass))
        return kResultFalse;

    cold_ = clamp01(values[0]);
    ice_ = clamp01(values[1]);
    metal_ = clamp01(values[2]);
    frost_ = clamp01(values[3]);
    shiver_ = clamp01(values[4]);
    space_ = clamp01(values[5]);
    output_ = clamp01(values[6]);
    bypass_ = bypass != 0;

    return kResultOk;
}

tresult PLUGIN_API Processor::getState(IBStream* state)
{
    if (!state)
        return kResultFalse;

    IBStreamer stream(state, kLittleEndian);
    const float values[kComponentStateValueCount] = {
        cold_, ice_, metal_, frost_, shiver_, space_, output_
    };

    return writeComponentStatePayload(stream, values, bypass_ ? 1 : 0)
        ? kResultOk
        : kResultFalse;
}

} // namespace Colderator
