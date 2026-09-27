#include "processor.h"
#include "ids.h"
#include "parameters.h"
#include "state_format.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace Colderator {

namespace {
constexpr float kPi = 3.14159265358979323846f;

inline float clamp01(float v)
{
    return std::max(0.f, std::min(1.f, v));
}

inline float normalizedOutputToDb(float v)
{
    return -12.f + 24.f * clamp01(v);
}

inline float onePoleCoeff(double sampleRate, float hz)
{
    return std::exp(-2.f * kPi * hz / static_cast<float>(sampleRate));
}

inline float zapDenormal(float v)
{
    return std::fabs(v) < 1.0e-30f ? 0.f : v;
}

inline int materialIndex(float v)
{
    return std::max(0, std::min(kMaterialCount - 1,
        static_cast<int>(std::lround(clamp01(v) * static_cast<float>(kMaterialCount - 1)))));
}

inline int atmosphereIndex(float v)
{
    return std::max(0, std::min(kAtmosphereTypeCount - 1,
        static_cast<int>(std::lround(clamp01(v) * static_cast<float>(kAtmosphereTypeCount - 1)))));
}

inline float materialWet(float v)
{
    v = clamp01(v);
    if (v <= 0.20f)
        return 1.25f * v; // 20% -> 25% transformed material
    if (v <= 0.50f)
        return 0.25f + (v - 0.20f) * (0.47f / 0.30f); // 50% -> 72%
    if (v <= 0.75f)
        return 0.72f + (v - 0.50f) * (0.16f / 0.25f); // 75% -> 88%
    return 0.88f + (v - 0.75f) * (0.12f / 0.25f);     // 100% -> 100%
}
}

void Processor::Resonator::setBandpass(double sampleRate, float frequency, float q)
{
    const float fs = static_cast<float>(std::max(8000.0, sampleRate));
    const float f = std::max(20.f, std::min(frequency, fs * 0.45f));
    const float safeQ = std::max(0.25f, q);
    const float w0 = 2.f * kPi * f / fs;
    const float alpha = std::sin(w0) / (2.f * safeQ);
    const float a0 = 1.f + alpha;

    b0 = alpha / a0;
    b2 = -alpha / a0;
    a1 = (-2.f * std::cos(w0)) / a0;
    a2 = (1.f - alpha) / a0;
}

float Processor::Resonator::process(float x)
{
    const float y = b0 * x + z1;
    z1 = zapDenormal(-a1 * y + z2);
    z2 = zapDenormal(b2 * x - a2 * y);

    if (!std::isfinite(y) || !std::isfinite(z1) || !std::isfinite(z2))
    {
        clear();
        return 0.f;
    }
    return y;
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

tresult PLUGIN_API Processor::setupProcessing(ProcessSetup& setup)
{
    sampleRate_ = setup.sampleRate > 1.0 ? setup.sampleRate : 44100.0;
    resetDsp();
    return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API Processor::setActive(TBool state)
{
    if (state)
        resetDsp();
    return AudioEffect::setActive(state);
}

tresult PLUGIN_API Processor::canProcessSampleSize(int32 symbolicSampleSize)
{
    return symbolicSampleSize == kSample32 ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API Processor::setBusArrangements(SpeakerArrangement* inputs, int32 numIns,
                                                  SpeakerArrangement* outputs, int32 numOuts)
{
    if (numIns == 1 && numOuts == 1)
    {
        const auto in = inputs[0];
        const auto out = outputs[0];
        const bool stereo = in == SpeakerArr::kStereo && out == SpeakerArr::kStereo;
        const bool mono = in == SpeakerArr::kMono && out == SpeakerArr::kMono;
        if (stereo || mono)
            return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
    }

    return kResultFalse;
}

void Processor::resetDsp()
{
    lowState_.fill(0.f);
    midLowState_.fill(0.f);
    fastEnv_.fill(0.f);
    slowEnv_.fill(0.f);
    frostPrevNoise_.fill(0.f);
    frostRng_[0] = 0x125A91u;
    frostRng_[1] = 0xC01D77u;
    spaceLowState_.fill(0.f);
    frostHeld_.fill(0.f);
    frostHoldCounter_.fill(0);
    textureRng_[0] = 0x1CE5A11u;
    textureRng_[1] = 0x57EE1A2u;
    iceShardEnv_.fill(0.f);
    metalParticleEnv_.fill(0.f);
    frostCrackleEnv_.fill(0.f);
    windNoiseState_.fill(0.f);
    windGust_.fill(0.f);
    windGustTarget_.fill(0.f);
    windGustCounter_.fill(0);
    cinematicLowState_.fill(0.f);
    cinematicBloomState_.fill(0.f);
    cinematicMotionState_.fill(0.f);
    atmosphereRng_[0][0] = 0xA7105A1u;
    atmosphereRng_[0][1] = 0xA7105B2u;
    atmosphereRng_[1][0] = 0xB7105C3u;
    atmosphereRng_[1][1] = 0xB7105D4u;
    for (auto& slot : atmosphereNoiseLow_) slot.fill(0.f);
    for (auto& slot : atmosphereNoiseHighPrev_) slot.fill(0.f);
    for (auto& slot : atmosphereGust_) slot.fill(0.f);
    for (auto& slot : atmosphereSwell_) slot.fill(0.f);
    for (auto& slot : atmosphereEventEnv_) slot.fill(0.f);
    for (auto& slot : atmospherePhase_) slot.fill(0.f);
    for (int ch = 0; ch < kChannels; ++ch)
    {
        spaceBuffer_[ch].assign(static_cast<size_t>(sampleRate_ * 0.18) + 8u, 0.f);
        spaceWrite_[ch] = 0;
        iceDelayBuffer_[ch].assign(static_cast<size_t>(sampleRate_ * 0.012) + 8u, 0.f);
        iceDelayWrite_[ch] = 0;
        shiverDelayBuffer_[ch].assign(static_cast<size_t>(sampleRate_ * 0.020) + 8u, 0.f);
        shiverDelayWrite_[ch] = 0;
        cinematicBuffer_[ch].assign(static_cast<size_t>(sampleRate_ * 1.35) + 16u, 0.f);
        cinematicWrite_[ch] = 0;
        shiverJitter_[ch] = 0.f;
        shiverJitterTarget_[ch] = 0.f;
        shiverJitterCounter_[ch] = 0;
        shiverShiftedLowState_[ch] = 0.f;
        shiverShiftedLowState2_[ch] = 0.f;
        shiverDryLowState_[ch] = 0.f;
        shiverDryLowState2_[ch] = 0.f;
        shiverPreLowState_[ch] = 0.f;
        shiverPreLowState2_[ch] = 0.f;
        shiverResultLowState_[ch] = 0.f;
        shiverResultLowState2_[ch] = 0.f;
    }
    shiverPhaseA_ = 0.f;
    shiverPhaseB_ = 0.f;
    metalPhaseA_ = 0.f;
    metalPhaseB_ = 0.f;
    resonatorUpdateCounter_ = 0;
    lastResonatorIce_ = -1.f;
    lastResonatorMetal_ = -1.f;

    for (auto& channel : iceModes_)
        for (auto& mode : channel)
            mode.clear();

    for (auto& channel : metalModes_)
        for (auto& mode : channel)
            mode.clear();

    smCold_ = cold_;
    smIce_ = ice_;
    smMetal_ = metal_;
    smFrost_ = frost_;
    smShiver_ = shiver_;
    smSpace_ = space_;
    smOutput_ = output_;
    smAtmosphereAAmount_ = atmosphereAAmount_;
    smAtmosphereBAmount_ = atmosphereBAmount_;

    const float effectiveIceBlock = clamp01(smCold_ * smIce_);
    const float effectiveMetalBlock = clamp01(smCold_ * smMetal_);
    updateResonators(effectiveIceBlock, effectiveMetalBlock);
}

void Processor::updateResonators(float ice, float metal)
{
    lastResonatorIce_ = ice;
    lastResonatorMetal_ = metal;
    const float iceExtreme = clamp01((ice - 0.90f) / 0.10f);
    const float metalExtreme = clamp01((metal - 0.90f) / 0.10f);

    const float iceQ = 0.75f + 2.6f * ice + 16.f * iceExtreme * iceExtreme;
    const float metalQ = 0.70f + 3.4f * metal + 22.f * metalExtreme * metalExtreme;

    constexpr float iceHz[kMaterialCount][kIceModes] = {
        {2410.f, 3670.f, 5530.f, 8210.f}, // Crystal
        {1780.f, 2960.f, 4720.f, 7480.f}, // Glass
        {3290.f, 4870.f, 6910.f, 9340.f}, // Crack
        {620.f, 1180.f, 2390.f, 5110.f},  // Black Ice
        {1540.f, 3080.f, 6160.f, 9120.f}, // Icicle
        {2710.f, 4210.f, 6820.f, 10300.f} // Shatter
    };
    constexpr float metalHz[kMaterialCount][kMetalModes] = {
        {710.f, 1230.f, 2070.f, 3490.f, 5870.f},  // Steel
        {220.f, 510.f, 930.f, 1670.f, 2890.f},    // Pipe
        {430.f, 860.f, 1410.f, 2380.f, 4010.f},   // Chain
        {180.f, 740.f, 1910.f, 3670.f, 6530.f},   // Sheet
        {310.f, 970.f, 1880.f, 4120.f, 7210.f},   // Machine
        {260.f, 620.f, 1320.f, 2740.f, 4980.f}    // Rust
    };

    const int iceModel = std::max(0, std::min(kMaterialCount - 1, iceMaterial_));
    const int metalModel = std::max(0, std::min(kMaterialCount - 1, metalMaterial_));
    lastIceMaterial_ = iceModel;
    lastMetalMaterial_ = metalModel;

    for (int ch = 0; ch < kChannels; ++ch)
    {
        const float stereoSkew = ch == 0 ? 0.993f : 1.007f;

        for (int i = 0; i < kIceModes; ++i)
            iceModes_[ch][i].setBandpass(sampleRate_, iceHz[iceModel][i] * stereoSkew, iceQ);

        for (int i = 0; i < kMetalModes; ++i)
            metalModes_[ch][i].setBandpass(sampleRate_, metalHz[metalModel][i] * stereoSkew, metalQ);
    }
}

void Processor::applyParameter(ParamID id, float normalized)
{
    if (!std::isfinite(normalized))
        return;

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
        case kIceMaterial: iceMaterial_ = materialIndex(v); break;
        case kMetalMaterial: metalMaterial_ = materialIndex(v); break;
        case kFrostMaterial: frostMaterial_ = materialIndex(v); break;
        case kShiverMaterial: shiverMaterial_ = materialIndex(v); break;
        case kSpaceMaterial: spaceMaterial_ = materialIndex(v); break;
        case kAtmosAType: atmosphereAType_ = atmosphereIndex(v); break;
        case kAtmosAAmount: atmosphereAAmount_ = v; break;
        case kAtmosBType: atmosphereBType_ = atmosphereIndex(v); break;
        case kAtmosBAmount: atmosphereBAmount_ = v; break;
        default: break;
    }
}

tresult PLUGIN_API Processor::process(ProcessData& data)
{
    constexpr int32 kMaxAutomationQueues = 16;
    IParamValueQueue* automationQueues[kMaxAutomationQueues] {};
    int32 automationIndices[kMaxAutomationQueues] {};
    int32 automationCounts[kMaxAutomationQueues] {};
    int32 automationQueueCount = 0;

    if (data.inputParameterChanges)
    {
        const int32 count = std::min<int32>(data.inputParameterChanges->getParameterCount(),
                                            kMaxAutomationQueues);
        for (int32 i = 0; i < count; ++i)
        {
            if (auto* queue = data.inputParameterChanges->getParameterData(i))
            {
                automationQueues[automationQueueCount] = queue;
                automationIndices[automationQueueCount] = 0;
                automationCounts[automationQueueCount] = queue->getPointCount();
                ++automationQueueCount;
            }
        }
    }

    if (data.numSamples <= 0)
    {
        for (int32 q = 0; q < automationQueueCount; ++q)
        {
            auto* queue = automationQueues[q];
            const int32 pointCount = automationCounts[q];
            if (!queue || pointCount <= 0)
                continue;

            int32 sampleOffset = 0;
            ParamValue value = 0.0;
            if (queue->getPoint(pointCount - 1, sampleOffset, value) == kResultTrue)
                applyParameter(queue->getParameterId(), static_cast<float>(value));
        }
        return kResultOk;
    }

    if (data.numInputs < 1 || data.numOutputs < 1)
        return kResultOk;

    if (data.symbolicSampleSize != kSample32)
        return kResultFalse;

    const auto& inBus = data.inputs[0];
    auto& outBus = data.outputs[0];
    if (!inBus.channelBuffers32 || !outBus.channelBuffers32)
        return kResultOk;

    const int32 channels = std::min<int32>(
        std::min(inBus.numChannels, outBus.numChannels), kChannels);

    const float smooth = 1.f - std::exp(-1.f / static_cast<float>(sampleRate_ * 0.015));
    const float lowA = onePoleCoeff(sampleRate_, 520.f);
    const float deepA = onePoleCoeff(sampleRate_, 145.f);
    const float fastA = onePoleCoeff(sampleRate_, 95.f);
    const float slowA = onePoleCoeff(sampleRate_, 12.f);

    for (int32 sample = 0; sample < data.numSamples; ++sample)
    {
        for (int32 q = 0; q < automationQueueCount; ++q)
        {
            auto* queue = automationQueues[q];
            int32& pointIndex = automationIndices[q];
            const int32 pointCount = automationCounts[q];

            while (pointIndex < pointCount)
            {
                int32 sampleOffset = 0;
                ParamValue value = 0.0;
                if (queue->getPoint(pointIndex, sampleOffset, value) != kResultTrue)
                {
                    ++pointIndex;
                    continue;
                }

                if (sampleOffset > sample)
                    break;

                applyParameter(queue->getParameterId(), static_cast<float>(value));
                ++pointIndex;
            }
        }

        // Global parameter smoothing advances exactly once per sample so both
        // stereo channels see the same parameter state.
        smCold_ += (cold_ - smCold_) * smooth;
        smIce_ += (ice_ - smIce_) * smooth;
        smMetal_ += (metal_ - smMetal_) * smooth;
        smFrost_ += (frost_ - smFrost_) * smooth;
        smShiver_ += (shiver_ - smShiver_) * smooth;
        smSpace_ += (space_ - smSpace_) * smooth;
        smOutput_ += (output_ - smOutput_) * smooth;
        smAtmosphereAAmount_ += (atmosphereAAmount_ - smAtmosphereAAmount_) * smooth;
        smAtmosphereBAmount_ += (atmosphereBAmount_ - smAtmosphereBAmount_) * smooth;

        shiverPhaseA_ += 2.f * kPi * 4.7f / static_cast<float>(sampleRate_);
        shiverPhaseB_ += 2.f * kPi * 7.9f / static_cast<float>(sampleRate_);
        metalPhaseA_ += 2.f * kPi * 1133.f / static_cast<float>(sampleRate_);
        metalPhaseB_ += 2.f * kPi * 1777.f / static_cast<float>(sampleRate_);
        if (shiverPhaseA_ >= 2.f * kPi) shiverPhaseA_ -= 2.f * kPi;
        if (shiverPhaseB_ >= 2.f * kPi) shiverPhaseB_ -= 2.f * kPi;
        if (metalPhaseA_ >= 2.f * kPi) metalPhaseA_ -= 2.f * kPi;
        if (metalPhaseB_ >= 2.f * kPi) metalPhaseB_ -= 2.f * kPi;
        const float shiverMod = 0.62f * std::sin(shiverPhaseA_) +
                                0.38f * std::sin(shiverPhaseB_);
        const float metalCarrier = 0.58f * std::sin(metalPhaseA_) +
                                   0.42f * std::sin(metalPhaseB_);

        // COLD is the global transformation intensity.
        // Individual modules are participation weights: 0% means truly excluded.
        const float effectiveIce = clamp01(smCold_ * smIce_);
        const float effectiveMetal = clamp01(smCold_ * smMetal_);
        const float effectiveFrost = clamp01(smCold_ * smFrost_);
        const float effectiveShiver = clamp01(smCold_ * smShiver_);
        const float effectiveSpace = clamp01(smCold_ * smSpace_);

        // Tonal COLD shaping belongs to the material modules. SHIVER is
        // motion and SPACE is environment; enabling either alone must not
        // silently reshape the source low end before their own DSP runs.
        const float tonalParticipation = std::max(
            {smIce_, smMetal_, smFrost_});
        const float effectiveCold = clamp01(smCold_ * tonalParticipation);

        if (resonatorUpdateCounter_ <= 0)
        {
            constexpr float kResonatorUpdateEpsilon = 1.0e-4f;
            if (std::fabs(effectiveIce - lastResonatorIce_) > kResonatorUpdateEpsilon ||
                std::fabs(effectiveMetal - lastResonatorMetal_) > kResonatorUpdateEpsilon ||
                iceMaterial_ != lastIceMaterial_ || metalMaterial_ != lastMetalMaterial_)
            {
                updateResonators(effectiveIce, effectiveMetal);
            }
            resonatorUpdateCounter_ = 15;
        }
        else
        {
            --resonatorUpdateCounter_;
        }

        const float iceExtreme = clamp01((effectiveIce - 0.90f) / 0.10f);
        const float metalExtreme = clamp01((effectiveMetal - 0.90f) / 0.10f);
        const float iceMix = effectiveIce * (0.48f + 0.92f * iceExtreme);
        const float metalMix = effectiveMetal * (0.58f + 1.32f * metalExtreme);
        const float outputGain = std::pow(10.f, normalizedOutputToDb(smOutput_) / 20.f);

        const int iceModel = std::max(0, std::min(kMaterialCount - 1, iceMaterial_));
        const int metalModel = std::max(0, std::min(kMaterialCount - 1, metalMaterial_));
        const int frostModel = std::max(0, std::min(kMaterialCount - 1, frostMaterial_));
        const int shiverModel = std::max(0, std::min(kMaterialCount - 1, shiverMaterial_));
        const int spaceModel = std::max(0, std::min(kMaterialCount - 1, spaceMaterial_));

        constexpr float iceEventScale[kMaterialCount] = {1.00f, 0.55f, 1.75f, 0.35f, 0.75f, 2.20f};
        constexpr float iceDecayMs[kMaterialCount] = {6.5f, 11.0f, 2.8f, 18.0f, 8.0f, 1.7f};
        constexpr float iceShardGain[kMaterialCount] = {1.00f, 0.55f, 1.35f, 0.40f, 0.80f, 1.65f};
        constexpr float iceGlassGain[kMaterialCount] = {1.00f, 1.35f, 0.55f, 0.72f, 1.10f, 0.62f};

        constexpr float metalEventScale[kMaterialCount] = {1.00f, 0.45f, 1.65f, 0.65f, 1.20f, 1.85f};
        constexpr float metalDecayMs[kMaterialCount] = {26.f, 58.f, 18.f, 42.f, 24.f, 12.f};
        constexpr float metalParticleGain[kMaterialCount] = {1.00f, 0.72f, 1.30f, 0.90f, 1.15f, 1.45f};
        constexpr float metalSidebandGain[kMaterialCount] = {1.00f, 0.55f, 0.82f, 1.25f, 1.50f, 0.68f};

        constexpr float frostHoldBaseMs[kMaterialCount] = {0.025f, 0.060f, 0.018f, 0.010f, 0.085f, 0.140f};
        constexpr float frostHoldRangeMs[kMaterialCount] = {0.190f, 0.280f, 0.120f, 0.060f, 0.360f, 0.520f};
        constexpr float frostCrackleScale[kMaterialCount] = {1.00f, 0.45f, 2.20f, 1.70f, 0.70f, 1.25f};
        constexpr float frostNoiseGain[kMaterialCount] = {1.00f, 0.55f, 0.82f, 1.55f, 0.72f, 0.42f};
        constexpr float frostHeldGain[kMaterialCount] = {1.00f, 1.18f, 0.62f, 0.48f, 1.28f, 1.42f};

        constexpr float shiverBaseMs[kMaterialCount] = {1.6f, 2.5f, 3.4f, 4.0f, 5.2f, 1.2f};
        constexpr float shiverDepthScale[kMaterialCount] = {0.72f, 0.42f, 0.95f, 1.35f, 1.65f, 0.28f};
        constexpr float shiverWindGain[kMaterialCount] = {0.18f, 0.72f, 0.95f, 1.25f, 1.65f, 0.38f};
        constexpr float shiverJitterGain[kMaterialCount] = {1.20f, 0.45f, 0.85f, 1.05f, 0.72f, 1.65f};
        constexpr float shiverGustMinSec[kMaterialCount] = {0.045f, 0.120f, 0.080f, 0.045f, 0.160f, 0.030f};

        constexpr float spaceD1Ms[kMaterialCount] = {7.3f, 18.0f, 4.8f, 12.5f, 24.0f, 31.0f};
        constexpr float spaceD2Ms[kMaterialCount] = {13.7f, 34.0f, 9.7f, 25.0f, 41.0f, 57.0f};
        constexpr float spaceD3Ms[kMaterialCount] = {23.9f, 61.0f, 17.0f, 43.0f, 72.0f, 91.0f};
        constexpr float spaceFbMs[kMaterialCount] = {41.7f, 83.0f, 31.0f, 69.0f, 109.0f, 137.0f};
        constexpr float spaceHpHz[kMaterialCount] = {420.f, 280.f, 560.f, 520.f, 360.f, 240.f};
        constexpr float spaceFeedbackBias[kMaterialCount] = {0.00f, 0.12f, -0.08f, 0.06f, 0.10f, 0.16f};
        constexpr float spaceEarlyScale[kMaterialCount] = {1.00f, 0.72f, 1.35f, 0.92f, 0.60f, 0.48f};

        for (int32 ch = 0; ch < channels; ++ch)
        {
            const float* in = inBus.channelBuffers32[ch];
            float* out = outBus.channelBuffers32[ch];
            if (!in || !out)
                continue;

            const float x = in[sample];

            lowState_[ch] = zapDenormal(lowA * lowState_[ch] + (1.f - lowA) * x);
            midLowState_[ch] = zapDenormal(deepA * midLowState_[ch] + (1.f - deepA) * x);

            const float lowMid = lowState_[ch] - midLowState_[ch];
            const float highDetail = x - lowState_[ch];

            const float absX = std::fabs(x);
            fastEnv_[ch] = zapDenormal(fastA * fastEnv_[ch] + (1.f - fastA) * absX);
            slowEnv_[ch] = zapDenormal(slowA * slowEnv_[ch] + (1.f - slowA) * absX);
            const float transient = std::max(0.f, fastEnv_[ch] - slowEnv_[ch]);
            const float transientNorm = transient / (0.02f + slowEnv_[ch]);

            const float coldCurve = effectiveCold * effectiveCold;
            const float coldCut = 0.72f * effectiveCold + 0.22f * coldCurve;
            const float edgeAmount = (0.32f * effectiveCold + 0.40f * coldCurve) *
                                     clamp01(0.28f + transientNorm * 1.9f);
            float y = x - lowMid * coldCut + highDetail * edgeAmount;

            const float transientExcitation = clamp01(0.28f + transientNorm * 2.5f);
            const float iceInput = highDetail * (0.55f + 0.95f * transientExcitation);
            const float metalInput = x * (0.35f + 1.15f * transientExcitation);

            // Shared deterministic material-texture source. It only creates audible
            // output when input energy exists; silence remains silence.
            unsigned int tr = textureRng_[ch];
            tr ^= tr << 13;
            tr ^= tr >> 17;
            tr ^= tr << 5;
            textureRng_[ch] = tr;
            const float textureWhite =
                (static_cast<float>(tr & 0x00FFFFFFu) / 8388607.5f) - 1.f;
            const float sourceActivity =
                clamp01(4.0f * slowEnv_[ch] + 1.8f * transientNorm);
            const float sourceScale =
                std::min(1.f, 2.4f * slowEnv_[ch] + 0.7f * std::fabs(highDetail));

            // ICE particles: transient-biased crystal shards.
            const float iceEventProb =
                effectiveIce * sourceActivity * iceEventScale[iceModel] *
                (0.00035f + 0.0025f * transientExcitation);
            const float iceRandom01 =
                static_cast<float>((tr >> 8) & 0x0000FFFFu) / 65535.f;
            if (iceRandom01 < iceEventProb)
                iceShardEnv_[ch] = 1.f;
            const float iceShardDecay =
                std::exp(-1.f / static_cast<float>(sampleRate_ * (iceDecayMs[iceModel] * 0.001f)));
            iceShardEnv_[ch] = zapDenormal(iceShardEnv_[ch] * iceShardDecay);
            const float iceShard =
                textureWhite * iceShardEnv_[ch] * sourceScale;

            // METAL particles: rarer, longer mechanical impacts.
            const float metalEventProb =
                effectiveMetal * sourceActivity * metalEventScale[metalModel] *
                (0.00010f + 0.00075f * transientExcitation);
            const float metalRandom01 =
                static_cast<float>((tr >> 1) & 0x0000FFFFu) / 65535.f;
            if (metalRandom01 < metalEventProb)
                metalParticleEnv_[ch] = 1.f;
            const float metalParticleDecay =
                std::exp(-1.f / static_cast<float>(sampleRate_ * (metalDecayMs[metalModel] * 0.001f)));
            metalParticleEnv_[ch] = zapDenormal(metalParticleEnv_[ch] * metalParticleDecay);
            const float metalParticle =
                textureWhite * metalParticleEnv_[ch] * sourceScale;

            // FROST crackles: many tiny irregular surface events.
            const float frostEventProb =
                effectiveFrost * sourceActivity * 0.0035f * frostCrackleScale[frostModel];
            const float frostRandom01 =
                static_cast<float>((tr >> 16) & 0x0000FFFFu) / 65535.f;
            if (frostRandom01 < frostEventProb)
                frostCrackleEnv_[ch] = 1.f;
            const float frostCrackleDecay =
                std::exp(-1.f / static_cast<float>(sampleRate_ * 0.0018));
            frostCrackleEnv_[ch] = zapDenormal(frostCrackleEnv_[ch] * frostCrackleDecay);
            const float frostCrackle =
                textureWhite * frostCrackleEnv_[ch] * sourceScale;

            // SHIVER wind: a slowly changing gust envelope over filtered noise.
            if (windGustCounter_[ch] <= 0)
            {
                windGustTarget_[ch] =
                    0.15f + 0.85f * std::fabs(textureWhite);
                windGustCounter_[ch] = std::max(
                    1, static_cast<int>(sampleRate_ *
                    (shiverGustMinSec[shiverModel] +
                     0.110f * (1.f - effectiveShiver))));
            }
            else
            {
                --windGustCounter_[ch];
            }
            windGust_[ch] +=
                (windGustTarget_[ch] - windGust_[ch]) * 0.00085f;
            windNoiseState_[ch] = zapDenormal(
                0.965f * windNoiseState_[ch] + 0.035f * textureWhite);
            const float windTexture =
                windNoiseState_[ch] * windGust_[ch] * sourceScale;

            float iceSignal = 0.f;
            for (auto& mode : iceModes_[ch])
                iceSignal += mode.process(iceInput);
            iceSignal *= 1.f / static_cast<float>(kIceModes);

            float metalSignal = 0.f;
            for (auto& mode : metalModes_[ch])
                metalSignal += mode.process(metalInput);
            metalSignal *= 1.f / static_cast<float>(kMetalModes);

            // ICE = crystal / broken glass / icicles.
            // It is a material morph, not a small parallel colour layer.
            float glass = 0.f;
            auto& iceDelay = iceDelayBuffer_[ch];
            if (!iceDelay.empty())
            {
                const float side = ch == 0 ? 0.97f : 1.03f;
                const int size = static_cast<int>(iceDelay.size());
                const int w = iceDelayWrite_[ch];
                const int d1 = std::max(1, static_cast<int>(sampleRate_ * 0.00073 * side));
                const int d2 = std::max(1, static_cast<int>(sampleRate_ * 0.00131 / side));
                const int d3 = std::max(1, static_cast<int>(sampleRate_ * 0.00217 * side));
                const int d4 = std::max(1, static_cast<int>(sampleRate_ * 0.00491 / side));
                auto readIce = [&](int delay) {
                    int index = w - delay;
                    while (index < 0) index += size;
                    return iceDelay[static_cast<size_t>(index)];
                };

                glass = 0.72f * readIce(d1) - 0.53f * readIce(d2) +
                        0.41f * readIce(d3) - 0.27f * readIce(d4);
                const float write = zapDenormal(highDetail + 0.28f * glass * effectiveIce);
                iceDelay[static_cast<size_t>(w)] = std::isfinite(write) ? write : 0.f;
                iceDelayWrite_[ch] = (w + 1 >= size) ? 0 : (w + 1);
            }
            const float iceTexture = 0.10f * x +
                                     1.18f * iceGlassGain[iceModel] * glass +
                                     iceSignal * (1.22f + 0.66f * iceExtreme) +
                                     iceShard * iceShardGain[iceModel] *
                                         (0.36f + 0.58f * effectiveIce);
            const float iceWet = materialWet(effectiveIce);
            y = y * (1.f - iceWet) + iceTexture * iceWet;

            // METAL = steel / pipes / machinery.
            // Inharmonic resonances and sidebands increasingly replace the source.
            const float metalSideband = highDetail * metalCarrier;
            const float metalTexture = 0.08f * x +
                                       metalSignal * (1.38f + 0.92f * metalExtreme) +
                                       metalSideband * metalSidebandGain[metalModel] *
                                           (0.66f + 0.46f * metalExtreme) +
                                       metalParticle * metalCarrier * metalParticleGain[metalModel] *
                                           (0.48f + 0.62f * effectiveMetal);
            const float metalWet = materialWet(effectiveMetal);
            y = y * (1.f - metalWet) + metalTexture * metalWet;

            // FROST: deterministic, signal-dependent high-frequency texture.
            // No input energy means no frost output, even though the RNG state advances.
            unsigned int r = frostRng_[ch];
            r ^= r << 13;
            r ^= r >> 17;
            r ^= r << 5;
            frostRng_[ch] = r;
            const float white = (static_cast<float>(r & 0x00FFFFFFu) / 8388607.5f) - 1.f;
            const float frostNoise = 0.5f * (white - frostPrevNoise_[ch]);
            frostPrevNoise_[ch] = white;

            const float frostExtreme = clamp01((effectiveFrost - 0.90f) / 0.10f);
            const float frostDrive = effectiveFrost * (0.18f + 0.34f * frostExtreme);
            const float frostCarrier = 1.35f * std::fabs(highDetail) + 0.55f * slowEnv_[ch];

            const int holdSamples = std::max(1, static_cast<int>(
                sampleRate_ * ((frostHoldBaseMs[frostModel] +
                                frostHoldRangeMs[frostModel] * effectiveFrost) * 0.001f)));
            if (frostHoldCounter_[ch] <= 0)
            {
                frostHeld_[ch] = highDetail;
                frostHoldCounter_[ch] = holdSamples;
            }
            else
            {
                --frostHoldCounter_[ch];
            }
            const float microFreeze = frostHeld_[ch] - highDetail;
            const float frostTexture = 0.15f * x +
                                       frostHeld_[ch] * 0.82f * frostHeldGain[frostModel] +
                                       microFreeze * frostHeldGain[frostModel] *
                                           (0.64f + 0.34f * frostExtreme) +
                                       frostNoise * frostCarrier * frostNoiseGain[frostModel] *
                                           (0.34f + 0.42f * frostExtreme) +
                                       frostCrackle * frostCrackleScale[frostModel] *
                                           (0.52f + 0.58f * effectiveFrost);
            const float frostWet = materialWet(effectiveFrost);
            y = y * (1.f - frostWet) + frostTexture * frostWet;

            // SHIVER = wind / cold tremor / irregular micro-Doppler.
            // Use a moving short delay plus stepped deterministic jitter so
            // it feels physically unstable rather than like a clean tremolo.
            const float shiverExtreme = clamp01((effectiveShiver - 0.90f) / 0.10f);
            auto& shiverDelay = shiverDelayBuffer_[ch];
            if (!shiverDelay.empty())
            {
                const int size = static_cast<int>(shiverDelay.size());
                const int w = shiverDelayWrite_[ch];
                shiverDelay[static_cast<size_t>(w)] = x;

                if (shiverJitterCounter_[ch] <= 0)
                {
                    unsigned int jr = frostRng_[ch];
                    jr ^= jr << 13; jr ^= jr >> 17; jr ^= jr << 5;
                    frostRng_[ch] = jr;
                    shiverJitterTarget_[ch] =
                        (static_cast<float>(jr & 0x00FFFFFFu) / 8388607.5f) - 1.f;
                    shiverJitterCounter_[ch] = std::max(
                        1, static_cast<int>(sampleRate_ * (0.035 + 0.045 * (1.f - effectiveShiver))));
                }
                else
                {
                    --shiverJitterCounter_[ch];
                }

                shiverJitter_[ch] += (shiverJitterTarget_[ch] - shiverJitter_[ch]) * 0.0025f;
                const float side = ch == 0 ? 1.f : -1.f;
                const float motion =
                    (0.62f * shiverMod +
                     0.38f * shiverJitter_[ch] * side * shiverJitterGain[shiverModel]);
                const float baseMs =
                    shiverBaseMs[shiverModel] + 1.2f * effectiveShiver;
                const float depthMs =
                    shiverDepthScale[shiverModel] *
                    (0.8f + 2.8f * effectiveShiver + 3.8f * shiverExtreme);
                float delaySamples = static_cast<float>(sampleRate_) *
                                     (baseMs + depthMs * motion) * 0.001f;
                delaySamples = std::max(1.f, std::min(delaySamples, static_cast<float>(size - 3)));

                float readPos = static_cast<float>(w) - delaySamples;
                while (readPos < 0.f) readPos += static_cast<float>(size);
                const int i0 = static_cast<int>(readPos);
                const int i1 = (i0 + 1) % size;
                const float frac = readPos - static_cast<float>(i0);
                const float shifted = shiverDelay[static_cast<size_t>(i0)] * (1.f - frac) +
                                      shiverDelay[static_cast<size_t>(i1)] * frac;

                // SHIVER is now applied as a high-passed *difference*
                // relative to the incoming signal. This avoids low-band
                // reconstruction and the phase-summation boost it caused.
                const float shiverTarget =
                    (0.08f - 0.03f * shiverExtreme) * y +
                    (0.76f + 0.16f * shiverExtreme) * shifted +
                    highDetail * shiverJitter_[ch] *
                        (0.18f + 0.30f * shiverExtreme) +
                    windTexture * shiverWindGain[shiverModel] *
                        (0.16f + 0.30f * effectiveShiver);

                const float shiverDelta = shiverTarget - y;
                const float shiverProtectA = onePoleCoeff(sampleRate_, 220.f);

                // Four actual cascaded one-pole high-pass stages. Each stage
                // removes the low-passed component from the previous stage,
                // instead of subtracting a cascaded low-pass only once.
                shiverResultLowState_[ch] = zapDenormal(
                    shiverProtectA * shiverResultLowState_[ch] +
                    (1.f - shiverProtectA) * shiverDelta);
                const float hp1 = shiverDelta - shiverResultLowState_[ch];

                shiverResultLowState2_[ch] = zapDenormal(
                    shiverProtectA * shiverResultLowState2_[ch] +
                    (1.f - shiverProtectA) * hp1);
                const float hp2 = hp1 - shiverResultLowState2_[ch];

                shiverShiftedLowState_[ch] = zapDenormal(
                    shiverProtectA * shiverShiftedLowState_[ch] +
                    (1.f - shiverProtectA) * hp2);
                const float hp3 = hp2 - shiverShiftedLowState_[ch];

                shiverShiftedLowState2_[ch] = zapDenormal(
                    shiverProtectA * shiverShiftedLowState2_[ch] +
                    (1.f - shiverProtectA) * hp3);
                const float protectedDelta =
                    hp3 - shiverShiftedLowState2_[ch];
                const float shiverWet = materialWet(effectiveShiver);
                y += protectedDelta * shiverWet;
                shiverDelayWrite_[ch] = (w + 1 >= size) ? 0 : (w + 1);
            }

            // SPACE: sparse early reflections feed a deliberately cold,
            // low-diffusion feedback tail. The feedback is high-passed so the
            // decay stays lean/glassy instead of building warm low-mid bloom.
            const float spaceInput = y;
            auto& spaceBuffer = spaceBuffer_[ch];
            if (!spaceBuffer.empty())
            {
                const float side = ch == 0 ? 0.965f : 1.035f;
                const int d1 = std::max(1, static_cast<int>(
                    sampleRate_ * (spaceD1Ms[spaceModel] * 0.001f) * side));
                const int d2 = std::max(1, static_cast<int>(
                    sampleRate_ * (spaceD2Ms[spaceModel] * 0.001f) / side));
                const int d3 = std::max(1, static_cast<int>(
                    sampleRate_ * (spaceD3Ms[spaceModel] * 0.001f) * side));
                const int df = std::max(1, static_cast<int>(
                    sampleRate_ * (spaceFbMs[spaceModel] * 0.001f) / side));
                const int size = static_cast<int>(spaceBuffer.size());
                const int w = spaceWrite_[ch];

                auto readTap = [&](int delay) {
                    int index = w - delay;
                    while (index < 0) index += size;
                    return spaceBuffer[static_cast<size_t>(index)];
                };

                const float early = spaceEarlyScale[spaceModel] *
                                    (0.54f * readTap(d1) -
                                     0.29f * readTap(d2) +
                                     0.17f * readTap(d3));
                const float feedbackTap = readTap(df);

                const float lowAspace = onePoleCoeff(sampleRate_, spaceHpHz[spaceModel]);
                spaceLowState_[ch] = zapDenormal(lowAspace * spaceLowState_[ch] +
                                     (1.f - lowAspace) * feedbackTap);
                const float icyFeedback = feedbackTap - 0.82f * spaceLowState_[ch];

                const float spaceExtreme = clamp01((effectiveSpace - 0.90f) / 0.10f);
                const float feedback = 0.28f + 0.46f * effectiveSpace +
                                       0.10f * spaceExtreme +
                                       spaceFeedbackBias[spaceModel];

                const float roomWet =
                    early +
                    icyFeedback * (0.55f + 0.20f * effectiveSpace);
                const float spaceWet = materialWet(effectiveSpace);
                y = y * (1.f - spaceWet) + roomWet * spaceWet;

                const float writeValue = zapDenormal(spaceInput + icyFeedback * std::min(0.86f, feedback));
                spaceBuffer[static_cast<size_t>(w)] = std::isfinite(writeValue) ? writeValue : 0.f;
                spaceWrite_[ch] = (w + 1 >= size) ? 0 : (w + 1);
            }

            // CINEMATIC DEPTH LAYER
            // Fast material FX stay in the foreground; this layer creates
            // low-end weight, distant cloud and slow background motion.
            const float cinematicSceneParticipation = std::max(
                {0.35f * effectiveIce,
                 effectiveMetal,
                 0.65f * effectiveFrost,
                 0.15f * effectiveShiver,
                 0.85f * effectiveSpace});
            const float cinematicDepth =
                std::pow(clamp01((cinematicSceneParticipation - 0.22f) / 0.78f), 1.20f);
            if (cinematicDepth > 1.0e-5f)
            {
                // Low transient weight. Keep it controlled so it adds scale
                // without turning into a permanent bass boost.
                const float lowWeightA = onePoleCoeff(sampleRate_, 115.f);
                cinematicLowState_[ch] = zapDenormal(
                    lowWeightA * cinematicLowState_[ch] +
                    (1.f - lowWeightA) * x);
                const float impactDrive = clamp01(transientNorm * 1.8f);
                const float cinematicWeightParticipation = std::max(
                    {0.35f * effectiveIce,
                     effectiveMetal,
                     0.65f * effectiveFrost,
                     0.85f * effectiveSpace});
                const float spaceWetForCinematic = materialWet(effectiveSpace);
                const float weight =
                    cinematicLowState_[ch] * impactDrive *
                    cinematicWeightParticipation *
                    (1.f - spaceWetForCinematic) *
                    (0.22f + 0.38f * cinematicDepth);

                auto& cinBuffer = cinematicBuffer_[ch];
                if (!cinBuffer.empty())
                {
                    const int size = static_cast<int>(cinBuffer.size());
                    const int w = cinematicWrite_[ch];
                    const float side = ch == 0 ? 0.94f : 1.06f;

                    const int d1 = std::max(1, static_cast<int>(
                        sampleRate_ * 0.118f * side));
                    const int d2 = std::max(1, static_cast<int>(
                        sampleRate_ * 0.247f / side));
                    const int d3 = std::max(1, static_cast<int>(
                        sampleRate_ * 0.463f * side));
                    const int df = std::max(1, static_cast<int>(
                        sampleRate_ * 0.731f / side));

                    auto readCin = [&](int delay) {
                        int index = w - delay;
                        while (index < 0) index += size;
                        return cinBuffer[static_cast<size_t>(index)];
                    };

                    const float distant =
                        0.46f * readCin(d1) -
                        0.31f * readCin(d2) +
                        0.24f * readCin(d3);
                    const float feedbackTap = readCin(df);

                    // Slow bloom and motion are intentionally much slower than
                    // SHIVER so the rear field feels cinematic, not glitchy.
                    const float bloomA = onePoleCoeff(sampleRate_, 7.5f);
                    cinematicBloomState_[ch] = zapDenormal(
                        bloomA * cinematicBloomState_[ch] +
                        (1.f - bloomA) * distant);

                    const float motionA = onePoleCoeff(sampleRate_, 0.55f);
                    const float motionTarget =
                        (ch == 0 ? 1.f : -1.f) *
                        (0.65f * shiverMod + 0.35f * windGust_[ch]);
                    cinematicMotionState_[ch] = zapDenormal(
                        motionA * cinematicMotionState_[ch] +
                        (1.f - motionA) * motionTarget);

                    const float cloud =
                        distant * (0.42f + 0.24f * cinematicDepth) +
                        cinematicBloomState_[ch] *
                            (0.34f + 0.28f * cinematicDepth) +
                        feedbackTap * cinematicMotionState_[ch] *
                            (0.10f + 0.16f * cinematicDepth);

                    y += weight * cinematicDepth;
                    y += cloud * cinematicDepth;

                    const float cinInput =
                        0.52f * y +
                        0.28f * highDetail +
                        0.20f * cinematicLowState_[ch];
                    const float cinFeedback =
                        feedbackTap * (0.36f + 0.30f * cinematicDepth);
                    const float writeValue =
                        zapDenormal(cinInput + cinFeedback);
                    cinBuffer[static_cast<size_t>(w)] =
                        std::isfinite(writeValue) ? writeValue : 0.f;
                    cinematicWrite_[ch] = (w + 1 >= size) ? 0 : (w + 1);
                }
            }
            else
            {
                // Drain the long buffer while the cinematic layer is disabled,
                // preventing stale clouds from reappearing on later automation.
                auto& cinBuffer = cinematicBuffer_[ch];
                if (!cinBuffer.empty())
                {
                    const int size = static_cast<int>(cinBuffer.size());
                    const int w = cinematicWrite_[ch];
                    cinBuffer[static_cast<size_t>(w)] = 0.f;
                    cinematicWrite_[ch] = (w + 1 >= size) ? 0 : (w + 1);
                }
                cinematicBloomState_[ch] *= 0.9995f;
                cinematicMotionState_[ch] *= 0.9995f;
                cinematicLowState_[ch] *= 0.9995f;
            }

            // ATMOSPHERE SLOTS
            // Two independent, source-responsive cinematic generators. They are
            // not ordinary loops: the input envelope and COLD determine when
            // and how strongly the atmosphere appears.
            const float atmosphereAmounts[2] = {
                smAtmosphereAAmount_, smAtmosphereBAmount_
            };
            const int atmosphereTypes[2] = {
                atmosphereAType_, atmosphereBType_
            };

            for (int slot = 0; slot < 2; ++slot)
            {
                const float amount = clamp01(atmosphereAmounts[slot]);
                if (amount <= 1.0e-5f)
                    continue;

                const int type = std::max(0, std::min(
                    kAtmosphereTypeCount - 1, atmosphereTypes[slot]));

                unsigned int ar = atmosphereRng_[slot][ch];
                ar ^= ar << 13; ar ^= ar >> 17; ar ^= ar << 5;
                atmosphereRng_[slot][ch] = ar;
                const float n =
                    (static_cast<float>(ar & 0x00FFFFFFu) / 8388607.5f) - 1.f;

                const float coldDrive = 0.25f + 0.75f * smCold_;
                const float activity = clamp01(2.8f * slowEnv_[ch] + 1.2f * transientNorm);

                atmosphereNoiseLow_[slot][ch] = zapDenormal(
                    0.985f * atmosphereNoiseLow_[slot][ch] + 0.015f * n);
                const float highNoise =
                    0.5f * (n - atmosphereNoiseHighPrev_[slot][ch]);
                atmosphereNoiseHighPrev_[slot][ch] = n;

                atmosphereGust_[slot][ch] = zapDenormal(
                    0.9992f * atmosphereGust_[slot][ch] +
                    0.0008f * std::fabs(n));
                atmosphereSwell_[slot][ch] = zapDenormal(
                    0.9996f * atmosphereSwell_[slot][ch] +
                    0.0004f * activity);

                atmospherePhase_[slot][ch] +=
                    2.f * kPi *
                    (slot == 0 ? 0.21f : 0.29f) /
                    static_cast<float>(sampleRate_);
                if (atmospherePhase_[slot][ch] >= 2.f * kPi)
                    atmospherePhase_[slot][ch] -= 2.f * kPi;

                const float slowMotion =
                    0.5f + 0.5f * std::sin(atmospherePhase_[slot][ch] +
                                           (ch == 0 ? 0.f : 1.7f));

                const float eventProbability =
                    activity * amount *
                    ((type == 4 || type == 5) ? 0.00055f : 0.00008f);
                const float random01 =
                    static_cast<float>((ar >> 8) & 0x0000FFFFu) / 65535.f;
                if (random01 < eventProbability)
                    atmosphereEventEnv_[slot][ch] = 1.f;

                const float eventDecayMs =
                    type == 4 ? 180.f : (type == 5 ? 28.f : 90.f);
                const float eventDecay = std::exp(
                    -1.f / static_cast<float>(sampleRate_ * eventDecayMs * 0.001f));
                atmosphereEventEnv_[slot][ch] = zapDenormal(
                    atmosphereEventEnv_[slot][ch] * eventDecay);

                float layer = 0.f;
                switch (type)
                {
                    case 0: // Wind
                        layer = atmosphereNoiseLow_[slot][ch] *
                                (0.22f + 0.78f * atmosphereGust_[slot][ch]) *
                                (0.35f + 0.65f * activity);
                        break;

                    case 1: // Storm
                        layer = (0.78f * atmosphereNoiseLow_[slot][ch] +
                                 0.22f * highNoise) *
                                (0.25f + 1.15f * atmosphereGust_[slot][ch]) *
                                (0.45f + 0.55f * slowMotion) *
                                (0.35f + 0.65f * activity);
                        break;

                    case 2: // Drone
                    {
                        const float sourceBody =
                            0.62f * lowState_[ch] + 0.38f * midLowState_[ch];
                        layer = sourceBody *
                                (0.72f + 0.28f * slowMotion) *
                                (0.65f + 0.35f * atmosphereSwell_[slot][ch]) *
                                (0.55f + 0.45f * activity);
                        break;
                    }

                    case 3: // Rumble
                    {
                        const float sourceBody =
                            0.45f * std::fabs(lowState_[ch]) +
                            0.55f * slowEnv_[ch];
                        layer = atmosphereNoiseLow_[slot][ch] *
                                sourceBody *
                                (0.85f + 0.35f * atmosphereGust_[slot][ch]) *
                                2.4f;
                        break;
                    }

                    case 4: // Distant Metal
                        layer = atmosphereEventEnv_[slot][ch] *
                                (0.58f * std::sin(metalPhaseA_ * 0.37f) +
                                 0.42f * std::sin(metalPhaseB_ * 0.23f)) *
                                (0.35f + 0.65f * activity);
                        break;

                    case 5: // Ice Cracks
                        layer = atmosphereEventEnv_[slot][ch] *
                                highNoise *
                                (0.55f + 0.45f * transientExcitation) * 1.8f;
                        break;

                    case 6: // Air
                        layer = highNoise *
                                (0.18f + 0.82f * atmosphereSwell_[slot][ch]) *
                                (0.30f + 0.70f * activity);
                        break;

                    case 7: // Ghost
                        layer = (0.64f * atmosphereNoiseLow_[slot][ch] +
                                 0.36f * highNoise) *
                                (0.25f + 0.75f * slowMotion) *
                                atmosphereSwell_[slot][ch];
                        break;

                    case 8: // Swell
                        layer = highDetail *
                                atmosphereSwell_[slot][ch] *
                                (0.35f + 0.65f * slowMotion);
                        break;

                    case 9: // Machine
                        layer = (0.55f * atmosphereNoiseLow_[slot][ch] +
                                 0.45f * highNoise) *
                                metalCarrier *
                                (0.35f + 0.65f * activity);
                        break;
                }

                const float slotGain =
                    amount * coldDrive *
                    (type == 2 || type == 3 ? 0.75f : 0.42f);
                y += layer * slotGain;
            }

            y *= outputGain;

            if (!std::isfinite(y))
                y = 0.f;

            out[sample] = bypass_ ? x : y;
        }
    }

    return kResultOk;
}


uint32 PLUGIN_API Processor::getTailSamples()
{
    // The cold SPACE feedback is intentionally audible but bounded. Report a
    // conservative two-second host tail so offline rendering and transport
    // stops do not truncate the designed decay at supported sample rates.
    const double samples = sampleRate_ * 2.0;
    return static_cast<uint32>(std::min<double>(samples,
                                                static_cast<double>(std::numeric_limits<uint32>::max())));
}

tresult PLUGIN_API Processor::setState(IBStream* state)
{
    if (!state)
        return kResultFalse;

    IBStreamer stream(state, kLittleEndian);
    ComponentStatePayload payload {};

    if (!readComponentStatePayload(stream, payload))
        return kResultFalse;

    cold_ = clamp01(payload.values[0]);
    ice_ = clamp01(payload.values[1]);
    metal_ = clamp01(payload.values[2]);
    frost_ = clamp01(payload.values[3]);
    shiver_ = clamp01(payload.values[4]);
    space_ = clamp01(payload.values[5]);
    output_ = clamp01(payload.values[6]);
    bypass_ = payload.bypass != 0;

    iceMaterial_ = std::max(0, std::min(kMaterialCount - 1, static_cast<int>(payload.materials[0])));
    metalMaterial_ = std::max(0, std::min(kMaterialCount - 1, static_cast<int>(payload.materials[1])));
    frostMaterial_ = std::max(0, std::min(kMaterialCount - 1, static_cast<int>(payload.materials[2])));
    shiverMaterial_ = std::max(0, std::min(kMaterialCount - 1, static_cast<int>(payload.materials[3])));
    spaceMaterial_ = std::max(0, std::min(kMaterialCount - 1, static_cast<int>(payload.materials[4])));

    atmosphereAType_ = atmosphereIndex(payload.atmosphere[0]);
    atmosphereAAmount_ = clamp01(payload.atmosphere[1]);
    atmosphereBType_ = atmosphereIndex(payload.atmosphere[2]);
    atmosphereBAmount_ = clamp01(payload.atmosphere[3]);

    return kResultOk;
}

tresult PLUGIN_API Processor::getState(IBStream* state)
{
    if (!state)
        return kResultFalse;

    IBStreamer stream(state, kLittleEndian);
    ComponentStatePayload payload {};
    payload.values[0] = cold_;
    payload.values[1] = ice_;
    payload.values[2] = metal_;
    payload.values[3] = frost_;
    payload.values[4] = shiver_;
    payload.values[5] = space_;
    payload.values[6] = output_;
    payload.bypass = bypass_ ? 1 : 0;
    payload.materials[0] = iceMaterial_;
    payload.materials[1] = metalMaterial_;
    payload.materials[2] = frostMaterial_;
    payload.materials[3] = shiverMaterial_;
    payload.materials[4] = spaceMaterial_;
    payload.atmosphere[0] = static_cast<float>(atmosphereAType_) /
                            static_cast<float>(kAtmosphereTypeCount - 1);
    payload.atmosphere[1] = atmosphereAAmount_;
    payload.atmosphere[2] = static_cast<float>(atmosphereBType_) /
                            static_cast<float>(kAtmosphereTypeCount - 1);
    payload.atmosphere[3] = atmosphereBAmount_;

    return writeComponentStatePayload(stream, payload)
        ? kResultOk
        : kResultFalse;
}

} // namespace Colderator
