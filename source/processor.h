#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"

namespace Colderator {

class Processor final : public Steinberg::Vst::AudioEffect
{
public:
    Processor();
    static Steinberg::FUnknown* createInstance(void*)
    {
        return (Steinberg::Vst::IAudioProcessor*)new Processor();
    }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) override;
    Steinberg::tresult PLUGIN_API setBusArrangements(
        Steinberg::Vst::SpeakerArrangement* inputs,
        Steinberg::int32 numIns,
        Steinberg::Vst::SpeakerArrangement* outputs,
        Steinberg::int32 numOuts) override;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) override;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override;

private:
    void applyParameter(Steinberg::Vst::ParamID id, float normalized);

    float cold_ = 0.f;
    float ice_ = 0.f;
    float metal_ = 0.f;
    float frost_ = 0.f;
    float shiver_ = 0.f;
    float space_ = 0.f;
    float output_ = 0.5f; // normalized -12..+12 dB, 0 dB = 0.5
    bool bypass_ = false;
};

} // namespace Colderator
