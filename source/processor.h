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
    std::array<std::vector<float>, kChannels> spaceFarBuffer_ {};
    std::array<int, kChannels> spaceFarWrite_ {};
    std::array<float, kChannels> spaceFarLowState_ {};
    std::array<float, kChannels> spaceFarBloomState_ {};

    std::array<std::vector<float>, kChannels> iceDelayBuffer_ {};
    std::array<int, kChannels> iceDelayWrite_ {};
    std::array<std::vector<float>, kChannels> shiverDelayBuffer_ {};
    std::array<int, kChannels> shiverDelayWrite_ {};
    std::array<float, kChannels> shiverJitter_ {};
    std::array<float, kChannels> shiverJitterTarget_ {};
    std::array<int, kChannels> shiverJitterCounter_ {};
    std::array<float, kChannels> shiverShiftedLowState_ {};
    std::array<float, kChannels> shiverShiftedLowState2_ {};
    std::array<float, kChannels> shiverDryLowState_ {};
    std::array<float, kChannels> shiverDryLowState2_ {};
    std::array<float, kChannels> shiverPreLowState_ {};
    std::array<float, kChannels> shiverPreLowState2_ {};
    std::array<float, kChannels> shiverResultLowState_ {};
    std::array<float, kChannels> shiverResultLowState2_ {};
    std::array<float, kChannels> frostHeld_ {};
    std::array<int, kChannels> frostHoldCounter_ {};
    std::array<double, kChannels> frostCrackSamplePos_ {};
    std::array<double, kChannels> frostAirSamplePos_ {};
    std::array<float, kChannels> frostAirLowState_ {};

    // Generative material-texture state. All generators are deterministic,
    // signal-gated and allocation-free on the audio thread.
    std::array<unsigned int, kChannels> textureRng_ {{0x1CE5A11u, 0x57EE1A2u}};
    std::array<float, kChannels> iceShardEnv_ {};
    std::array<float, kChannels> metalParticleEnv_ {};
    std::array<double, kChannels> metalAirSamplePos_ {};
    std::array<double, kChannels> metalChimeSamplePos_ {};
    std::array<float, kChannels> frostCrackleEnv_ {};
    std::array<float, kChannels> windNoiseState_ {};
    std::array<float, kChannels> windGust_ {};
    std::array<float, kChannels> windGustTarget_ {};
    std::array<int, kChannels> windGustCounter_ {};

    // Cinematic depth layer: long background cloud + low impact weight.
    std::array<std::vector<float>, kChannels> cinematicBuffer_ {};
    std::array<int, kChannels> cinematicWrite_ {};
    std::array<float, kChannels> cinematicLowState_ {};
    std::array<float, kChannels> cinematicBloomState_ {};
    std::array<float, kChannels> cinematicMotionState_ {};

    // Two independent generative cinematic atmosphere slots.
    std::array<std::array<unsigned int, kChannels>, 2> atmosphereRng_ {{
        {{0xA7105A1u, 0xA7105B2u}},
        {{0xB7105C3u, 0xB7105D4u}}
    }};
    std::array<std::array<float, kChannels>, 2> atmosphereNoiseLow_ {};
    std::array<std::array<float, kChannels>, 2> atmosphereNoiseHighPrev_ {};
    std::array<std::array<float, kChannels>, 2> atmosphereGust_ {};
    std::array<std::array<float, kChannels>, 2> atmosphereSwell_ {};
    std::array<std::array<float, kChannels>, 2> atmosphereEventEnv_ {};
    std::array<std::array<float, kChannels>, 2> atmospherePhase_ {};
    std::array<std::array<std::vector<float>, kChannels>, 2> bloomBuffer_ {};
    std::array<std::array<int, kChannels>, 2> bloomWrite_ {};
    std::array<std::array<float, kChannels>, 2> bloomLowState_ {};
    std::array<std::array<float, kChannels>, 2> stormBodyState_ {};
    std::array<std::array<float, kChannels>, 2> stormPressureState_ {};
    std::array<std::array<float, kChannels>, 2> stormSnowState_ {};
    std::array<std::array<float, kChannels>, 2> stormImpactEnv_ {};
    std::array<std::array<double, kChannels>, 2> stormSamplePos_ {};
    std::array<std::array<double, kChannels>, 2> crackSamplePos_ {};
    std::array<std::array<double, kChannels>, 2> landscapeWindSamplePos_ {};
    std::array<std::array<double, kChannels>, 2> landscapeMetalSamplePos_ {};

    float shiverPhaseA_ = 0.f;
    float shiverPhaseB_ = 0.f;
    float cinematicPhaseA_ = 0.f;
    float cinematicPhaseB_ = 0.f;
    float metalPhaseA_ = 0.f;
    float metalPhaseB_ = 0.f;
    int resonatorUpdateCounter_ = 0;
    float lastResonatorIce_ = -1.f;
    float lastResonatorMetal_ = -1.f;
    int lastIceMaterial_ = -1;
    int lastMetalMaterial_ = -1;

    float cold_ = 0.f;
    float ice_ = 0.f;
    float metal_ = 0.f;
    float frost_ = 0.f;
    float shiver_ = 0.f;
    float space_ = 0.f;
    float output_ = 0.5f;
    bool bypass_ = false;

    int iceMaterial_ = 0;
    int metalMaterial_ = 0;
    int frostMaterial_ = 0;
    int shiverMaterial_ = 0;
    int spaceMaterial_ = 0;

    int atmosphereAType_ = 0;
    int atmosphereBType_ = 0;
    float atmosphereAAmount_ = 0.f;
    float atmosphereBAmount_ = 0.f;

    float smCold_ = 0.f;
    float smIce_ = 0.f;
    float smMetal_ = 0.f;
    float smFrost_ = 0.f;
    float smShiver_ = 0.f;
    float smSpace_ = 0.f;
    float smOutput_ = 0.5f;
    float smAtmosphereAAmount_ = 0.f;
    float smAtmosphereBAmount_ = 0.f;
};

} // namespace Colderator
