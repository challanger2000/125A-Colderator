#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include <array>
#include <vector>

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
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& setup) override;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) override;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) override;
    Steinberg::tresult PLUGIN_API setBusArrangements(
        Steinberg::Vst::SpeakerArrangement* inputs,
        Steinberg::int32 numIns,
        Steinberg::Vst::SpeakerArrangement* outputs,
        Steinberg::int32 numOuts) override;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) override;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override;
    Steinberg::uint32 PLUGIN_API getTailSamples() override;

#ifdef COLDERATOR_TESTING
    void setTestParameter(Steinberg::Vst::ParamID id, float value) { applyParameter(id, value); }
#endif

private:
    struct Resonator
    {
        float b0 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f;
        float z1 = 0.f, z2 = 0.f;

        void clear() { z1 = z2 = 0.f; }
        void setBandpass(double sampleRate, float frequency, float q);
        float process(float x);
    };

    static constexpr int kChannels = 2;
    static constexpr int kIceModes = 4;
    static constexpr int kMetalModes = 5;

    void applyParameter(Steinberg::Vst::ParamID id, float normalized);
    void resetDsp();
    void updateResonators(float ice, float metal);

    double sampleRate_ = 44100.0;

    std::array<std::array<Resonator, kIceModes>, kChannels> iceModes_ {};
    std::array<std::array<Resonator, kMetalModes>, kChannels> metalModes_ {};

    std::array<float, kChannels> lowState_ {};
    std::array<float, kChannels> midLowState_ {};
    std::array<float, kChannels> fastEnv_ {};
    std::array<float, kChannels> slowEnv_ {};
    std::array<float, kChannels> frostPrevNoise_ {};
    std::array<unsigned int, kChannels> frostRng_ {{0x125A91u, 0xC01D77u}};
    std::array<std::vector<float>, kChannels> spaceBuffer_ {};
    std::array<int, kChannels> spaceWrite_ {};
    std::array<float, kChannels> spaceLowState_ {};

    std::array<std::vector<float>, kChannels> iceDelayBuffer_ {};
    std::array<int, kChannels> iceDelayWrite_ {};
    std::array<float, kChannels> frostHeld_ {};
    std::array<int, kChannels> frostHoldCounter_ {};

    float shiverPhaseA_ = 0.f;
    float shiverPhaseB_ = 0.f;
    float metalPhaseA_ = 0.f;
    float metalPhaseB_ = 0.f;
    int resonatorUpdateCounter_ = 0;
    float lastResonatorIce_ = -1.f;
    float lastResonatorMetal_ = -1.f;

    float cold_ = 0.f;
    float ice_ = 0.f;
    float metal_ = 0.f;
    float frost_ = 0.f;
    float shiver_ = 0.f;
    float space_ = 0.f;
    float output_ = 0.5f;
    bool bypass_ = false;

    float smCold_ = 0.f;
    float smIce_ = 0.f;
    float smMetal_ = 0.f;
    float smFrost_ = 0.f;
    float smShiver_ = 0.f;
    float smSpace_ = 0.f;
    float smOutput_ = 0.5f;
};

} // namespace Colderator
