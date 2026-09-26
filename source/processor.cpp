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
    for (int ch = 0; ch < kChannels; ++ch)
    {
        spaceBuffer_[ch].assign(static_cast<size_t>(sampleRate_ * 0.18) + 8u, 0.f);
        spaceWrite_[ch] = 0;
    }
    shiverPhaseA_ = 0.f;
    shiverPhaseB_ = 0.f;
    resonatorUpdateCounter_ = 0;

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

    const float coldIceBlock = 0.78f * std::pow(clamp01((smCold_ - 0.12f) / 0.88f), 1.30f);
    const float coldMetalBlock = 0.68f * std::pow(clamp01((smCold_ - 0.22f) / 0.78f), 1.35f);
    updateResonators(clamp01(smIce_ + coldIceBlock),
                     clamp01(smMetal_ + coldMetalBlock));
}

void Processor::updateResonators(float ice, float metal)
{
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
        if (shiverPhaseA_ >= 2.f * kPi) shiverPhaseA_ -= 2.f * kPi;
        if (shiverPhaseB_ >= 2.f * kPi) shiverPhaseB_ -= 2.f * kPi;
        const float shiverMod = 0.62f * std::sin(shiverPhaseA_) +
                                0.38f * std::sin(shiverPhaseB_);

        const float coldIce = 0.78f * std::pow(clamp01((smCold_ - 0.12f) / 0.88f), 1.30f);
        const float coldMetal = 0.68f * std::pow(clamp01((smCold_ - 0.22f) / 0.78f), 1.35f);
        const float coldFrost = 0.82f * std::pow(clamp01((smCold_ - 0.34f) / 0.66f), 1.15f);
        const float coldShiver = 0.55f * std::pow(clamp01((smCold_ - 0.48f) / 0.52f), 1.20f);
        const float coldSpace = 0.72f * std::pow(clamp01((smCold_ - 0.42f) / 0.58f), 1.20f);

        const float effectiveIce = clamp01(smIce_ + coldIce);
        const float effectiveMetal = clamp01(smMetal_ + coldMetal);
        const float effectiveFrost = clamp01(smFrost_ + coldFrost);
        const float effectiveShiver = clamp01(smShiver_ + coldShiver);
        const float effectiveSpace = clamp01(smSpace_ + coldSpace);

        if (resonatorUpdateCounter_ <= 0)
        {
            updateResonators(effectiveIce, effectiveMetal);
            resonatorUpdateCounter_ = 15;
        }
        else
        {
            --resonatorUpdateCounter_;
        }

        const float iceExtreme = clamp01((effectiveIce - 0.90f) / 0.10f);
        const float metalExtreme = clamp01((effectiveMetal - 0.90f) / 0.10f);
        const float iceMix = effectiveIce * (0.18f + 0.34f * iceExtreme);
        const float metalMix = effectiveMetal * (0.22f + 0.52f * metalExtreme);
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

            const float coldCut = 0.52f * smCold_;
            const float edgeAmount = 0.22f * smCold_ * clamp01(transientNorm * 1.7f);
            float y = x - lowMid * coldCut + highDetail * edgeAmount;

            const float transientExcitation = clamp01(0.15f + transientNorm * 2.2f);
            const float iceInput = highDetail * (0.25f + 0.75f * transientExcitation);
            const float metalInput = x * (0.12f + 0.88f * transientExcitation);

            float iceSignal = 0.f;
            for (auto& mode : iceModes_[ch])
                iceSignal += mode.process(iceInput);
            iceSignal *= 1.f / static_cast<float>(kIceModes);

            float metalSignal = 0.f;
            for (auto& mode : metalModes_[ch])
                metalSignal += mode.process(metalInput);
            metalSignal *= 1.f / static_cast<float>(kMetalModes);

            y += iceSignal * iceMix;
            y += metalSignal * metalMix;

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
            const float frostDrive = effectiveFrost * (0.055f + 0.095f * frostExtreme);
            const float frostCarrier = std::fabs(highDetail) + 0.35f * slowEnv_[ch];
            y += frostNoise * frostCarrier * frostDrive;

            // SHIVER: shallow dual-rate spectral tremor. It modulates only the
            // high-detail component, so the fundamental is not frequency-shifted.
            const float shiverExtreme = clamp01((effectiveShiver - 0.90f) / 0.10f);
            const float shiverDepth = effectiveShiver * (0.018f + 0.032f * shiverExtreme);
            y += highDetail * shiverMod * shiverDepth;

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
