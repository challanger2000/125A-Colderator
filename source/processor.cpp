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
    for (int ch = 0; ch < kChannels; ++ch)
    {
        spaceBuffer_[ch].assign(static_cast<size_t>(sampleRate_ * 0.18) + 8u, 0.f);
        spaceWrite_[ch] = 0;
        iceDelayBuffer_[ch].assign(static_cast<size_t>(sampleRate_ * 0.012) + 8u, 0.f);
        iceDelayWrite_[ch] = 0;
        shiverDelayBuffer_[ch].assign(static_cast<size_t>(sampleRate_ * 0.020) + 8u, 0.f);
        shiverDelayWrite_[ch] = 0;
        shiverJitter_[ch] = 0.f;
        shiverJitterTarget_[ch] = 0.f;
        shiverJitterCounter_[ch] = 0;
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

    constexpr float iceHz[kIceModes] = { 2410.f, 3670.f, 5530.f, 8210.f };
    constexpr float metalHz[kMetalModes] = { 710.f, 1230.f, 2070.f, 3490.f, 5870.f };

    for (int ch = 0; ch < kChannels; ++ch)
    {
        const float stereoSkew = ch == 0 ? 0.993f : 1.007f;

        for (int i = 0; i < kIceModes; ++i)
            iceModes_[ch][i].setBandpass(sampleRate_, iceHz[i] * stereoSkew, iceQ);

        for (int i = 0; i < kMetalModes; ++i)
            metalModes_[ch][i].setBandpass(sampleRate_, metalHz[i] * stereoSkew, metalQ);
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

        const float moduleParticipation = std::max(
            {smIce_, smMetal_, smFrost_, smShiver_, smSpace_});
        const float effectiveCold = clamp01(smCold_ * moduleParticipation);

        if (resonatorUpdateCounter_ <= 0)
        {
            constexpr float kResonatorUpdateEpsilon = 1.0e-4f;
            if (std::fabs(effectiveIce - lastResonatorIce_) > kResonatorUpdateEpsilon ||
                std::fabs(effectiveMetal - lastResonatorMetal_) > kResonatorUpdateEpsilon)
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
                effectiveIce * sourceActivity * (0.00035f + 0.0025f * transientExcitation);
            const float iceRandom01 =
                static_cast<float>((tr >> 8) & 0x0000FFFFu) / 65535.f;
            if (iceRandom01 < iceEventProb)
                iceShardEnv_[ch] = 1.f;
            const float iceShardDecay =
                std::exp(-1.f / static_cast<float>(sampleRate_ * 0.0065));
            iceShardEnv_[ch] = zapDenormal(iceShardEnv_[ch] * iceShardDecay);
            const float iceShard =
                textureWhite * iceShardEnv_[ch] * sourceScale;

            // METAL particles: rarer, longer mechanical impacts.
            const float metalEventProb =
                effectiveMetal * sourceActivity * (0.00010f + 0.00075f * transientExcitation);
            const float metalRandom01 =
                static_cast<float>((tr >> 1) & 0x0000FFFFu) / 65535.f;
            if (metalRandom01 < metalEventProb)
                metalParticleEnv_[ch] = 1.f;
            const float metalParticleDecay =
                std::exp(-1.f / static_cast<float>(sampleRate_ * 0.026));
            metalParticleEnv_[ch] = zapDenormal(metalParticleEnv_[ch] * metalParticleDecay);
            const float metalParticle =
                textureWhite * metalParticleEnv_[ch] * sourceScale;

            // FROST crackles: many tiny irregular surface events.
            const float frostEventProb =
                effectiveFrost * sourceActivity * 0.0035f;
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
                    (0.055 + 0.110 * (1.f - effectiveShiver))));
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
            const float iceTexture = 0.10f * x + 1.18f * glass +
                                     iceSignal * (1.22f + 0.66f * iceExtreme) +
                                     iceShard * (0.36f + 0.58f * effectiveIce);
            const float iceWet = materialWet(effectiveIce);
            y = y * (1.f - iceWet) + iceTexture * iceWet;

            // METAL = steel / pipes / machinery.
            // Inharmonic resonances and sidebands increasingly replace the source.
            const float metalSideband = highDetail * metalCarrier;
            const float metalTexture = 0.08f * x +
                                       metalSignal * (1.38f + 0.92f * metalExtreme) +
                                       metalSideband * (0.66f + 0.46f * metalExtreme) +
                                       metalParticle * metalCarrier *
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
                sampleRate_ * (0.000025 + 0.00019 * effectiveFrost)));
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
                                       frostHeld_[ch] * 0.82f +
                                       microFreeze * (0.64f + 0.34f * frostExtreme) +
                                       frostNoise * frostCarrier *
                                           (0.34f + 0.42f * frostExtreme) +
                                       frostCrackle *
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
                const float motion = 0.62f * shiverMod +
                                     0.38f * shiverJitter_[ch] * side;
                const float baseMs = 1.6f + 1.6f * effectiveShiver;
                const float depthMs = 0.8f + 2.8f * effectiveShiver +
                                      3.8f * shiverExtreme;
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

                const float shiverTexture = (0.16f - 0.08f * shiverExtreme) * x +
                                            (0.78f + 0.16f * shiverExtreme) * shifted +
                                            highDetail * shiverJitter_[ch] *
                                                (0.20f + 0.34f * shiverExtreme) +
                                            windTexture *
                                                (0.18f + 0.34f * effectiveShiver);
                const float shiverWet = materialWet(effectiveShiver);
                y = y * (1.f - shiverWet) + shiverTexture * shiverWet;
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
                const int d1 = std::max(1, static_cast<int>(sampleRate_ * 0.0073 * side));
                const int d2 = std::max(1, static_cast<int>(sampleRate_ * 0.0137 / side));
                const int d3 = std::max(1, static_cast<int>(sampleRate_ * 0.0239 * side));
                const int df = std::max(1, static_cast<int>(sampleRate_ * 0.0417 / side));
                const int size = static_cast<int>(spaceBuffer.size());
                const int w = spaceWrite_[ch];

                auto readTap = [&](int delay) {
                    int index = w - delay;
                    while (index < 0) index += size;
                    return spaceBuffer[static_cast<size_t>(index)];
                };

                const float early = 0.54f * readTap(d1) -
                                    0.29f * readTap(d2) +
                                    0.17f * readTap(d3);
                const float feedbackTap = readTap(df);

                const float lowAspace = onePoleCoeff(sampleRate_, 420.f);
                spaceLowState_[ch] = zapDenormal(lowAspace * spaceLowState_[ch] +
                                     (1.f - lowAspace) * feedbackTap);
                const float icyFeedback = feedbackTap - 0.82f * spaceLowState_[ch];

                const float spaceExtreme = clamp01((effectiveSpace - 0.90f) / 0.10f);
                const float spaceMix = effectiveSpace * (0.18f + 0.24f * spaceExtreme);
                const float feedback = 0.28f + 0.46f * effectiveSpace +
                                       0.10f * spaceExtreme;
                y += (early + 0.55f * icyFeedback) * spaceMix;

                const float writeValue = zapDenormal(spaceInput + icyFeedback * std::min(0.86f, feedback));
                spaceBuffer[static_cast<size_t>(w)] = std::isfinite(writeValue) ? writeValue : 0.f;
                spaceWrite_[ch] = (w + 1 >= size) ? 0 : (w + 1);
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
