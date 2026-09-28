#include "processor.h"
#include "parameters.h"
#include "frozen_sources.h"

#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/common/memorystream.h"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Steinberg;
using namespace Steinberg::Vst;
using Colderator::Processor;

namespace {

constexpr double kPi = 3.14159265358979323846;

bool finiteBuffer(const std::vector<float>& x);
double meanAbsDiff(const std::vector<float>& a, const std::vector<float>& b, size_t skip);

struct Settings
{
    float cold = 0.f;
    float ice = 0.f;
    float metal = 0.f;
    float frost = 0.f;
    float shiver = 0.f;
    float space = 0.f;
    float iceMaterial = 0.f;
    float metalMaterial = 0.f;
    float frostMaterial = 0.f;
    float shiverMaterial = 0.f;
    float spaceMaterial = 0.f;
    float atmosphereAType = 0.f;
    float atmosphereAAmount = 0.f;
    float atmosphereBType = 0.f;
    float atmosphereBAmount = 0.f;
};


std::vector<float> renderChord(double sr, double seconds, const Settings& settings, int block = 128)
{
    Processor p;
    if (p.initialize(nullptr) != kResultOk)
        throw std::runtime_error("chord initialize failed");

    ProcessSetup setup {};
    setup.processMode = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = block;
    setup.sampleRate = sr;
    if (p.setupProcessing(setup) != kResultOk)
        throw std::runtime_error("chord setup failed");

    p.setTestParameter(Colderator::kCold, settings.cold);
    p.setTestParameter(Colderator::kIce, settings.ice);
    p.setTestParameter(Colderator::kMetal, settings.metal);
    p.setTestParameter(Colderator::kFrost, settings.frost);
    p.setTestParameter(Colderator::kShiver, settings.shiver);
    p.setTestParameter(Colderator::kSpace, settings.space);
    p.setTestParameter(Colderator::kIceMaterial, settings.iceMaterial);
    p.setTestParameter(Colderator::kMetalMaterial, settings.metalMaterial);
    p.setTestParameter(Colderator::kFrostMaterial, settings.frostMaterial);
    p.setTestParameter(Colderator::kShiverMaterial, settings.shiverMaterial);
    p.setTestParameter(Colderator::kSpaceMaterial, settings.spaceMaterial);
    p.setTestParameter(Colderator::kAtmosAType, settings.atmosphereAType);
    p.setTestParameter(Colderator::kAtmosAAmount, settings.atmosphereAAmount);
    p.setTestParameter(Colderator::kAtmosBType, settings.atmosphereBType);
    p.setTestParameter(Colderator::kAtmosBAmount, settings.atmosphereBAmount);
    p.setTestParameter(Colderator::kOutput, 0.5f);
    if (p.setActive(true) != kResultOk)
        throw std::runtime_error("chord active failed");

    const size_t total = static_cast<size_t>(std::llround(sr * seconds));
    std::vector<float> result(total, 0.f);
    std::vector<float> inL(block), inR(block), outL(block), outR(block);
    float* inPtrs[2] = {inL.data(), inR.data()};
    float* outPtrs[2] = {outL.data(), outR.data()};
    AudioBusBuffers inBus {};
    inBus.numChannels = 2;
    inBus.channelBuffers32 = inPtrs;
    AudioBusBuffers outBus {};
    outBus.numChannels = 2;
    outBus.channelBuffers32 = outPtrs;

    size_t pos = 0;
    while (pos < total)
    {
        const int n = static_cast<int>(std::min<size_t>(block, total - pos));
        for (int i = 0; i < n; ++i)
        {
            const double t = static_cast<double>(pos + static_cast<size_t>(i)) / sr;
            const float x = 0.075f * static_cast<float>(
                std::sin(2.0 * kPi * 220.0 * t) +
                std::sin(2.0 * kPi * 277.183 * t) +
                std::sin(2.0 * kPi * 329.628 * t));
            inL[i] = inR[i] = x;
            outL[i] = outR[i] = 0.f;
        }

        ProcessData data {};
        data.processMode = kRealtime;
        data.symbolicSampleSize = kSample32;
        data.numSamples = n;
        data.numInputs = 1;
        data.numOutputs = 1;
        data.inputs = &inBus;
        data.outputs = &outBus;
        if (p.process(data) != kResultOk)
            throw std::runtime_error("chord process failed");

        for (int i = 0; i < n; ++i)
            result[pos + static_cast<size_t>(i)] = outL[i];
        pos += static_cast<size_t>(n);
    }

    p.setActive(false);
    p.terminate();
    return result;
}

std::vector<float> renderSine(double sr, double seconds, double hz, const Settings& settings, int block = 128)
{

    Processor p;
    if (p.initialize(nullptr) != kResultOk)
        throw std::runtime_error("initialize failed");

    ProcessSetup setup {};
    setup.processMode = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = block;
    setup.sampleRate = sr;

    if (p.setupProcessing(setup) != kResultOk)
        throw std::runtime_error("setupProcessing failed");

    p.setTestParameter(Colderator::kCold, settings.cold);
    p.setTestParameter(Colderator::kIce, settings.ice);
    p.setTestParameter(Colderator::kMetal, settings.metal);
    p.setTestParameter(Colderator::kFrost, settings.frost);
    p.setTestParameter(Colderator::kShiver, settings.shiver);
    p.setTestParameter(Colderator::kSpace, settings.space);
    p.setTestParameter(Colderator::kIceMaterial, settings.iceMaterial);
    p.setTestParameter(Colderator::kMetalMaterial, settings.metalMaterial);
    p.setTestParameter(Colderator::kFrostMaterial, settings.frostMaterial);
    p.setTestParameter(Colderator::kShiverMaterial, settings.shiverMaterial);
    p.setTestParameter(Colderator::kSpaceMaterial, settings.spaceMaterial);
    p.setTestParameter(Colderator::kAtmosAType, settings.atmosphereAType);
    p.setTestParameter(Colderator::kAtmosAAmount, settings.atmosphereAAmount);
    p.setTestParameter(Colderator::kAtmosBType, settings.atmosphereBType);
    p.setTestParameter(Colderator::kAtmosBAmount, settings.atmosphereBAmount);
    p.setTestParameter(Colderator::kOutput, 0.5f);

    if (p.setActive(true) != kResultOk)
        throw std::runtime_error("setActive failed");

    const size_t total = static_cast<size_t>(std::llround(sr * seconds));
    std::vector<float> result(total, 0.f);

    std::vector<float> inL(block, 0.f), inR(block, 0.f), outL(block, 0.f), outR(block, 0.f);
    float* inPtrs[2] = { inL.data(), inR.data() };
    float* outPtrs[2] = { outL.data(), outR.data() };

    AudioBusBuffers inBus {};
    inBus.numChannels = 2;
    inBus.channelBuffers32 = inPtrs;

    AudioBusBuffers outBus {};
    outBus.numChannels = 2;
    outBus.channelBuffers32 = outPtrs;

    size_t pos = 0;
    while (pos < total)
    {
        const int n = static_cast<int>(std::min<size_t>(block, total - pos));

        for (int i = 0; i < n; ++i)
        {
            const double t = static_cast<double>(pos + static_cast<size_t>(i)) / sr;
            const float x = 0.2f * static_cast<float>(std::sin(2.0 * kPi * hz * t));
            inL[i] = x;
            inR[i] = x;
            outL[i] = 0.f;
            outR[i] = 0.f;
        }

        ProcessData data {};
        data.processMode = kRealtime;
        data.symbolicSampleSize = kSample32;
        data.numSamples = n;
        data.numInputs = 1;
        data.numOutputs = 1;
        data.inputs = &inBus;
        data.outputs = &outBus;

        if (p.process(data) != kResultOk)
            throw std::runtime_error("process failed");

        for (int i = 0; i < n; ++i)
            result[pos + static_cast<size_t>(i)] = outL[i];

        pos += static_cast<size_t>(n);
    }

    p.setActive(false);
    p.terminate();
    return result;
}



std::vector<float> renderImpulseWithBypassWindow(double sr, int block,
                                                  int totalBlocks,
                                                  int bypassStartBlock,
                                                  int bypassEndBlock,
                                                  const Settings& settings)
{
    Processor p;
    if (p.initialize(nullptr) != kResultOk)
        throw std::runtime_error("initialize failed");

    ProcessSetup setup {};
    setup.processMode = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = block;
    setup.sampleRate = sr;

    if (p.setupProcessing(setup) != kResultOk)
        throw std::runtime_error("setupProcessing failed");

    p.setTestParameter(Colderator::kCold, settings.cold);
    p.setTestParameter(Colderator::kIce, settings.ice);
    p.setTestParameter(Colderator::kMetal, settings.metal);
    p.setTestParameter(Colderator::kFrost, settings.frost);
    p.setTestParameter(Colderator::kShiver, settings.shiver);
    p.setTestParameter(Colderator::kSpace, settings.space);
    p.setTestParameter(Colderator::kIceMaterial, settings.iceMaterial);
    p.setTestParameter(Colderator::kMetalMaterial, settings.metalMaterial);
    p.setTestParameter(Colderator::kFrostMaterial, settings.frostMaterial);
    p.setTestParameter(Colderator::kShiverMaterial, settings.shiverMaterial);
    p.setTestParameter(Colderator::kSpaceMaterial, settings.spaceMaterial);
    p.setTestParameter(Colderator::kAtmosAType, settings.atmosphereAType);
    p.setTestParameter(Colderator::kAtmosAAmount, settings.atmosphereAAmount);
    p.setTestParameter(Colderator::kAtmosBType, settings.atmosphereBType);
    p.setTestParameter(Colderator::kAtmosBAmount, settings.atmosphereBAmount);
    p.setTestParameter(Colderator::kOutput, 0.5f);

    if (p.setActive(true) != kResultOk)
        throw std::runtime_error("setActive failed");

    const size_t total = static_cast<size_t>(block * totalBlocks);
    std::vector<float> result(total, 0.f);
    std::vector<float> inL(block, 0.f), inR(block, 0.f), outL(block, 0.f), outR(block, 0.f);
    float* inPtrs[2] = { inL.data(), inR.data() };
    float* outPtrs[2] = { outL.data(), outR.data() };

    AudioBusBuffers inBus {};
    inBus.numChannels = 2;
    inBus.channelBuffers32 = inPtrs;

    AudioBusBuffers outBus {};
    outBus.numChannels = 2;
    outBus.channelBuffers32 = outPtrs;

    for (int b = 0; b < totalBlocks; ++b)
    {
        std::fill(inL.begin(), inL.end(), 0.f);
        std::fill(inR.begin(), inR.end(), 0.f);
        std::fill(outL.begin(), outL.end(), 0.f);
        std::fill(outR.begin(), outR.end(), 0.f);

        if (b == 0)
        {
            inL[0] = 1.f;
            inR[0] = 1.f;
        }

        if (b == bypassStartBlock)
            p.setTestParameter(Colderator::kBypass, 1.f);
        if (b == bypassEndBlock)
            p.setTestParameter(Colderator::kBypass, 0.f);

        ProcessData data {};
        data.processMode = kRealtime;
        data.symbolicSampleSize = kSample32;
        data.numSamples = block;
        data.numInputs = 1;
        data.numOutputs = 1;
        data.inputs = &inBus;
        data.outputs = &outBus;

        if (p.process(data) != kResultOk)
            throw std::runtime_error("process failed");

        const size_t base = static_cast<size_t>(b * block);
        for (int i = 0; i < block; ++i)
            result[base + static_cast<size_t>(i)] = outL[i];
    }

    p.setActive(false);
    p.terminate();
    return result;
}

std::vector<float> renderImpulse(double sr, double seconds, const Settings& settings, int block = 128)
{

    Processor p;
    if (p.initialize(nullptr) != kResultOk)
        throw std::runtime_error("initialize failed");

    ProcessSetup setup {};
    setup.processMode = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = block;
    setup.sampleRate = sr;

    if (p.setupProcessing(setup) != kResultOk)
        throw std::runtime_error("setupProcessing failed");

    p.setTestParameter(Colderator::kCold, settings.cold);
    p.setTestParameter(Colderator::kIce, settings.ice);
    p.setTestParameter(Colderator::kMetal, settings.metal);
    p.setTestParameter(Colderator::kFrost, settings.frost);
    p.setTestParameter(Colderator::kShiver, settings.shiver);
    p.setTestParameter(Colderator::kSpace, settings.space);
    p.setTestParameter(Colderator::kIceMaterial, settings.iceMaterial);
    p.setTestParameter(Colderator::kMetalMaterial, settings.metalMaterial);
    p.setTestParameter(Colderator::kFrostMaterial, settings.frostMaterial);
    p.setTestParameter(Colderator::kShiverMaterial, settings.shiverMaterial);
    p.setTestParameter(Colderator::kSpaceMaterial, settings.spaceMaterial);
    p.setTestParameter(Colderator::kAtmosAType, settings.atmosphereAType);
    p.setTestParameter(Colderator::kAtmosAAmount, settings.atmosphereAAmount);
    p.setTestParameter(Colderator::kAtmosBType, settings.atmosphereBType);
    p.setTestParameter(Colderator::kAtmosBAmount, settings.atmosphereBAmount);
    p.setTestParameter(Colderator::kOutput, 0.5f);

    if (p.setActive(true) != kResultOk)
        throw std::runtime_error("setActive failed");

    const size_t total = static_cast<size_t>(std::llround(sr * seconds));
    std::vector<float> result(total, 0.f);

    std::vector<float> inL(block, 0.f), inR(block, 0.f), outL(block, 0.f), outR(block, 0.f);
    float* inPtrs[2] = { inL.data(), inR.data() };
    float* outPtrs[2] = { outL.data(), outR.data() };

    AudioBusBuffers inBus {};
    inBus.numChannels = 2;
    inBus.channelBuffers32 = inPtrs;

    AudioBusBuffers outBus {};
    outBus.numChannels = 2;
    outBus.channelBuffers32 = outPtrs;

    bool sent = false;
    size_t pos = 0;
    while (pos < total)
    {
        const int n = static_cast<int>(std::min<size_t>(block, total - pos));
        std::fill(inL.begin(), inL.end(), 0.f);
        std::fill(inR.begin(), inR.end(), 0.f);
        std::fill(outL.begin(), outL.end(), 0.f);
        std::fill(outR.begin(), outR.end(), 0.f);

        if (!sent)
        {
            inL[0] = 1.f;
            inR[0] = 1.f;
            sent = true;
        }

        ProcessData data {};
        data.processMode = kRealtime;
        data.symbolicSampleSize = kSample32;
        data.numSamples = n;
        data.numInputs = 1;
        data.numOutputs = 1;
        data.inputs = &inBus;
        data.outputs = &outBus;

        if (p.process(data) != kResultOk)
            throw std::runtime_error("process failed");

        for (int i = 0; i < n; ++i)
            result[pos + static_cast<size_t>(i)] = outL[i];

        pos += static_cast<size_t>(n);
    }

    p.setActive(false);
    p.terminate();
    return result;
}


std::vector<float> renderAutomationPattern(double sr, int block)
{
    Processor p;
    if (p.initialize(nullptr) != kResultOk)
        throw std::runtime_error("initialize failed");

    ProcessSetup setup {};
    setup.processMode = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = block;
    setup.sampleRate = sr;

    if (p.setupProcessing(setup) != kResultOk)
        throw std::runtime_error("setupProcessing failed");

    p.setTestParameter(Colderator::kCold, 0.f);
    p.setTestParameter(Colderator::kIce, 0.15f);
    p.setTestParameter(Colderator::kMetal, 0.10f);
    p.setTestParameter(Colderator::kFrost, 0.05f);
    p.setTestParameter(Colderator::kShiver, 0.05f);
    p.setTestParameter(Colderator::kSpace, 0.10f);
    p.setTestParameter(Colderator::kOutput, 0.5f);
    p.setTestParameter(Colderator::kBypass, 0.f);

    if (p.setActive(true) != kResultOk)
        throw std::runtime_error("setActive failed");

    const size_t total = static_cast<size_t>(std::llround(sr * 0.18));
    std::vector<float> result(total, 0.f);
    std::vector<float> inL(block, 0.f), inR(block, 0.f), outL(block, 0.f), outR(block, 0.f);
    float* inPtrs[2] = { inL.data(), inR.data() };
    float* outPtrs[2] = { outL.data(), outR.data() };

    AudioBusBuffers inBus {};
    inBus.numChannels = 2;
    inBus.channelBuffers32 = inPtrs;

    AudioBusBuffers outBus {};
    outBus.numChannels = 2;
    outBus.channelBuffers32 = outPtrs;

    struct Event { size_t sample; ParamID id; ParamValue value; };
    const std::array<Event, 8> events {{
        {static_cast<size_t>(sr * 0.020), Colderator::kCold, 0.35},
        {static_cast<size_t>(sr * 0.045), Colderator::kCold, 0.82},
        {static_cast<size_t>(sr * 0.070), Colderator::kSpace, 0.75},
        {static_cast<size_t>(sr * 0.090), Colderator::kBypass, 1.0},
        {static_cast<size_t>(sr * 0.105), Colderator::kBypass, 0.0},
        {static_cast<size_t>(sr * 0.120), Colderator::kMetal, 0.65},
        {static_cast<size_t>(sr * 0.140), Colderator::kCold, 1.0},
        {static_cast<size_t>(sr * 0.155), Colderator::kFrost, 0.70}
    }};

    size_t pos = 0;
    while (pos < total)
    {
        const int n = static_cast<int>(std::min<size_t>(block, total - pos));
        for (int i = 0; i < n; ++i)
        {
            const double t = static_cast<double>(pos + static_cast<size_t>(i)) / sr;
            const float x = 0.2f * static_cast<float>(std::sin(2.0 * kPi * 440.0 * t));
            inL[i] = x;
            inR[i] = x;
            outL[i] = 0.f;
            outR[i] = 0.f;
        }

        ParameterChanges changes(8);
        for (const auto& e : events)
        {
            if (e.sample >= pos && e.sample < pos + static_cast<size_t>(n))
            {
                int32 queueIndex = 0;
                auto* queue = changes.addParameterData(e.id, queueIndex);
                int32 pointIndex = 0;
                queue->addPoint(static_cast<int32>(e.sample - pos), e.value, pointIndex);
            }
        }

        ProcessData data {};
        data.processMode = kRealtime;
        data.symbolicSampleSize = kSample32;
        data.numSamples = n;
        data.numInputs = 1;
        data.numOutputs = 1;
        data.inputs = &inBus;
        data.outputs = &outBus;
        data.inputParameterChanges = changes.getParameterCount() > 0 ? &changes : nullptr;

        if (p.process(data) != kResultOk)
            throw std::runtime_error("automation process failed");

        for (int i = 0; i < n; ++i)
            result[pos + static_cast<size_t>(i)] = outL[i];

        pos += static_cast<size_t>(n);
    }

    p.setActive(false);
    p.terminate();
    return result;
}

std::vector<float> renderSingleBlockBypassAutomation(double sr)
{
    constexpr int block = 128;
    Processor p;
    if (p.initialize(nullptr) != kResultOk)
        throw std::runtime_error("initialize failed");

    ProcessSetup setup {};
    setup.processMode = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = block;
    setup.sampleRate = sr;
    if (p.setupProcessing(setup) != kResultOk)
        throw std::runtime_error("setupProcessing failed");

    p.setTestParameter(Colderator::kCold, 0.85f);
    p.setTestParameter(Colderator::kSpace, 0.70f);
    p.setTestParameter(Colderator::kOutput, 0.5f);
    p.setTestParameter(Colderator::kBypass, 0.f);
    if (p.setActive(true) != kResultOk)
        throw std::runtime_error("setActive failed");

    std::vector<float> inL(block), inR(block), outL(block, 0.f), outR(block, 0.f);
    for (int i = 0; i < block; ++i)
    {
        const float x = 0.2f * static_cast<float>(std::sin(2.0 * kPi * 440.0 * i / sr));
        inL[i] = x;
        inR[i] = x;
    }

    float* inPtrs[2] = { inL.data(), inR.data() };
    float* outPtrs[2] = { outL.data(), outR.data() };
    AudioBusBuffers inBus {};
    inBus.numChannels = 2;
    inBus.channelBuffers32 = inPtrs;
    AudioBusBuffers outBus {};
    outBus.numChannels = 2;
    outBus.channelBuffers32 = outPtrs;

    ParameterChanges changes(1);
    int32 queueIndex = 0;
    auto* queue = changes.addParameterData(Colderator::kBypass, queueIndex);
    int32 pointIndex = 0;
    queue->addPoint(32, 1.0, pointIndex);
    queue->addPoint(96, 0.0, pointIndex);

    ProcessData data {};
    data.processMode = kRealtime;
    data.symbolicSampleSize = kSample32;
    data.numSamples = block;
    data.numInputs = 1;
    data.numOutputs = 1;
    data.inputs = &inBus;
    data.outputs = &outBus;
    data.inputParameterChanges = &changes;

    if (p.process(data) != kResultOk)
        throw std::runtime_error("bypass automation process failed");

    p.setActive(false);
    p.terminate();

    std::vector<float> packed(static_cast<size_t>(block * 2), 0.f);
    std::copy(outL.begin(), outL.end(), packed.begin());
    std::copy(inL.begin(), inL.end(), packed.begin() + block);
    return packed;
}


std::vector<float> renderConfiguredProcessor(Processor& p, double sr, double seconds, double hz, int block = 128)
{
    const size_t total = static_cast<size_t>(std::llround(sr * seconds));
    std::vector<float> result(total, 0.f);
    std::vector<float> inL(block, 0.f), inR(block, 0.f), outL(block, 0.f), outR(block, 0.f);
    float* inPtrs[2] = { inL.data(), inR.data() };
    float* outPtrs[2] = { outL.data(), outR.data() };

    AudioBusBuffers inBus {};
    inBus.numChannels = 2;
    inBus.channelBuffers32 = inPtrs;
    AudioBusBuffers outBus {};
    outBus.numChannels = 2;
    outBus.channelBuffers32 = outPtrs;

    size_t pos = 0;
    while (pos < total)
    {
        const int n = static_cast<int>(std::min<size_t>(block, total - pos));
        for (int i = 0; i < n; ++i)
        {
            const double t = static_cast<double>(pos + static_cast<size_t>(i)) / sr;
            const float x = 0.2f * static_cast<float>(std::sin(2.0 * kPi * hz * t));
            inL[i] = x;
            inR[i] = x;
            outL[i] = 0.f;
            outR[i] = 0.f;
        }

        ProcessData data {};
        data.processMode = kRealtime;
        data.symbolicSampleSize = kSample32;
        data.numSamples = n;
        data.numInputs = 1;
        data.numOutputs = 1;
        data.inputs = &inBus;
        data.outputs = &outBus;

        if (p.process(data) != kResultOk)
            throw std::runtime_error("configured process failed");

        for (int i = 0; i < n; ++i)
            result[pos + static_cast<size_t>(i)] = outL[i];

        pos += static_cast<size_t>(n);
    }

    return result;
}

bool stateRoundtripMatches(double sr)
{
    constexpr int block = 128;

    Processor source;
    Processor restored;
    if (source.initialize(nullptr) != kResultOk || restored.initialize(nullptr) != kResultOk)
        throw std::runtime_error("state processor initialize failed");

    ProcessSetup setup {};
    setup.processMode = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = block;
    setup.sampleRate = sr;

    if (source.setupProcessing(setup) != kResultOk || restored.setupProcessing(setup) != kResultOk)
        throw std::runtime_error("state setupProcessing failed");

    source.setTestParameter(Colderator::kCold, 0.73f);
    source.setTestParameter(Colderator::kIce, 0.41f);
    source.setTestParameter(Colderator::kMetal, 0.66f);
    source.setTestParameter(Colderator::kFrost, 0.32f);
    source.setTestParameter(Colderator::kShiver, 0.27f);
    source.setTestParameter(Colderator::kSpace, 0.58f);
    source.setTestParameter(Colderator::kIceMaterial, 0.40f);
    source.setTestParameter(Colderator::kMetalMaterial, 0.80f);
    source.setTestParameter(Colderator::kFrostMaterial, 0.20f);
    source.setTestParameter(Colderator::kShiverMaterial, 1.00f);
    source.setTestParameter(Colderator::kSpaceMaterial, 0.60f);
    source.setTestParameter(Colderator::kAtmosAType, 2.f / 9.f);
    source.setTestParameter(Colderator::kAtmosAAmount, 0.47f);
    source.setTestParameter(Colderator::kAtmosBType, 1.f / 9.f);
    source.setTestParameter(Colderator::kAtmosBAmount, 0.31f);
    source.setTestParameter(Colderator::kOutput, 0.63f);
    source.setTestParameter(Colderator::kBypass, 0.f);

    Steinberg::MemoryStream state;
    if (source.getState(&state) != kResultOk)
        throw std::runtime_error("getState failed");
    state.seek(0, Steinberg::IBStream::kIBSeekSet, nullptr);
    if (restored.setState(&state) != kResultOk)
        throw std::runtime_error("setState failed");

    if (source.setActive(true) != kResultOk || restored.setActive(true) != kResultOk)
        throw std::runtime_error("state setActive failed");

    const auto a = renderConfiguredProcessor(source, sr, 0.6, 440.0, block);
    const auto b = renderConfiguredProcessor(restored, sr, 0.6, 440.0, block);

    source.setActive(false);
    restored.setActive(false);
    source.terminate();
    restored.terminate();

    if (a.size() != b.size())
        return false;

    double diff = 0.0;
    for (size_t i = 0; i < a.size(); ++i)
        diff += std::fabs(static_cast<double>(a[i]) - static_cast<double>(b[i]));
    diff /= std::max<size_t>(1, a.size());

    return diff < 1e-7;
}


std::vector<float> renderMode(double sr, ProcessModes mode, const Settings& settings, int block = 128)
{
    Processor p;
    if (p.initialize(nullptr) != kResultOk)
        throw std::runtime_error("mode initialize failed");

    ProcessSetup setup {};
    setup.processMode = mode;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = block;
    setup.sampleRate = sr;
    if (p.setupProcessing(setup) != kResultOk)
        throw std::runtime_error("mode setupProcessing failed");

    p.setTestParameter(Colderator::kCold, settings.cold);
    p.setTestParameter(Colderator::kIce, settings.ice);
    p.setTestParameter(Colderator::kMetal, settings.metal);
    p.setTestParameter(Colderator::kFrost, settings.frost);
    p.setTestParameter(Colderator::kShiver, settings.shiver);
    p.setTestParameter(Colderator::kSpace, settings.space);
    p.setTestParameter(Colderator::kIceMaterial, settings.iceMaterial);
    p.setTestParameter(Colderator::kMetalMaterial, settings.metalMaterial);
    p.setTestParameter(Colderator::kFrostMaterial, settings.frostMaterial);
    p.setTestParameter(Colderator::kShiverMaterial, settings.shiverMaterial);
    p.setTestParameter(Colderator::kSpaceMaterial, settings.spaceMaterial);
    p.setTestParameter(Colderator::kAtmosAType, settings.atmosphereAType);
    p.setTestParameter(Colderator::kAtmosAAmount, settings.atmosphereAAmount);
    p.setTestParameter(Colderator::kAtmosBType, settings.atmosphereBType);
    p.setTestParameter(Colderator::kAtmosBAmount, settings.atmosphereBAmount);
    p.setTestParameter(Colderator::kOutput, 0.5f);
    if (p.setActive(true) != kResultOk)
        throw std::runtime_error("mode setActive failed");

    const size_t total = static_cast<size_t>(std::llround(sr * 0.5));
    std::vector<float> result(total, 0.f);
    std::vector<float> inL(block), inR(block), outL(block), outR(block);
    float* inPtrs[2] = {inL.data(), inR.data()};
    float* outPtrs[2] = {outL.data(), outR.data()};
    AudioBusBuffers inBus {};
    inBus.numChannels = 2;
    inBus.channelBuffers32 = inPtrs;
    AudioBusBuffers outBus {};
    outBus.numChannels = 2;
    outBus.channelBuffers32 = outPtrs;

    size_t pos = 0;
    while (pos < total)
    {
        const int n = static_cast<int>(std::min<size_t>(block, total - pos));
        for (int i = 0; i < n; ++i)
        {
            const double t = static_cast<double>(pos + static_cast<size_t>(i)) / sr;
            const float x = 0.17f * static_cast<float>(std::sin(2.0 * kPi * 311.13 * t)) +
                            0.09f * static_cast<float>(std::sin(2.0 * kPi * 733.0 * t));
            inL[i] = x;
            inR[i] = x;
            outL[i] = outR[i] = 0.f;
        }

        ProcessData data {};
        data.processMode = mode;
        data.symbolicSampleSize = kSample32;
        data.numSamples = n;
        data.numInputs = 1;
        data.numOutputs = 1;
        data.inputs = &inBus;
        data.outputs = &outBus;
        if (p.process(data) != kResultOk)
            throw std::runtime_error("mode process failed");

        for (int i = 0; i < n; ++i)
            result[pos + static_cast<size_t>(i)] = outL[i];
        pos += static_cast<size_t>(n);
    }

    p.setActive(false);
    p.terminate();
    return result;
}

bool lifecycleAndRateChangeStable()
{
    Processor p;
    if (p.initialize(nullptr) != kResultOk)
        return false;

    for (double sr : {44100.0, 96000.0, 48000.0})
    {
        ProcessSetup setup {};
        setup.processMode = kRealtime;
        setup.symbolicSampleSize = kSample32;
        setup.maxSamplesPerBlock = 64;
        setup.sampleRate = sr;
        if (p.setupProcessing(setup) != kResultOk)
            return false;

        p.setTestParameter(Colderator::kCold, 0.8f);
        p.setTestParameter(Colderator::kSpace, 0.8f);
        if (p.setActive(true) != kResultOk)
            return false;

        const auto out = renderConfiguredProcessor(p, sr, 0.08, 440.0, 64);
        if (!finiteBuffer(out))
            return false;

        if (p.setActive(false) != kResultOk)
            return false;
    }

    p.terminate();
    return true;
}

bool activeStateLoadStable(double sr)
{
    constexpr int block = 64;
    Processor p;
    Processor donor;
    if (p.initialize(nullptr) != kResultOk || donor.initialize(nullptr) != kResultOk)
        return false;

    ProcessSetup setup {};
    setup.processMode = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = block;
    setup.sampleRate = sr;
    if (p.setupProcessing(setup) != kResultOk || donor.setupProcessing(setup) != kResultOk)
        return false;

    p.setTestParameter(Colderator::kCold, 0.15f);
    p.setTestParameter(Colderator::kSpace, 0.10f);
    donor.setTestParameter(Colderator::kCold, 0.92f);
    donor.setTestParameter(Colderator::kIce, 0.70f);
    donor.setTestParameter(Colderator::kMetal, 0.75f);
    donor.setTestParameter(Colderator::kFrost, 0.55f);
    donor.setTestParameter(Colderator::kShiver, 0.45f);
    donor.setTestParameter(Colderator::kSpace, 0.80f);
    donor.setTestParameter(Colderator::kOutput, 0.5f);

    if (p.setActive(true) != kResultOk)
        return false;
    const auto before = renderConfiguredProcessor(p, sr, 0.08, 440.0, block);

    Steinberg::MemoryStream state;
    if (donor.getState(&state) != kResultOk)
        return false;
    state.seek(0, Steinberg::IBStream::kIBSeekSet, nullptr);
    if (p.setState(&state) != kResultOk)
        return false;

    const auto after = renderConfiguredProcessor(p, sr, 0.20, 440.0, block);
    const bool ok = finiteBuffer(after) && meanAbsDiff(before, after, 0) > 1e-4;

    p.setActive(false);
    p.terminate();
    donor.terminate();
    return ok;
}

struct CpuStats
{
    double p95Us = 0.0;
    double p99Us = 0.0;
    double maxUs = 0.0;
    double deadlineUs = 0.0;
};

CpuStats measureCpu(double sr, int block)
{
    Processor p;
    if (p.initialize(nullptr) != kResultOk)
        throw std::runtime_error("cpu initialize failed");

    ProcessSetup setup {};
    setup.processMode = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = block;
    setup.sampleRate = sr;
    if (p.setupProcessing(setup) != kResultOk)
        throw std::runtime_error("cpu setup failed");

    p.setTestParameter(Colderator::kCold, 1.f);
    p.setTestParameter(Colderator::kIce, 1.f);
    p.setTestParameter(Colderator::kMetal, 1.f);
    p.setTestParameter(Colderator::kFrost, 1.f);
    p.setTestParameter(Colderator::kShiver, 1.f);
    p.setTestParameter(Colderator::kCold, 1.f);
    p.setTestParameter(Colderator::kCold, 1.f);
    p.setTestParameter(Colderator::kCold, 1.f);
    p.setTestParameter(Colderator::kSpace, 1.f);
    p.setTestParameter(Colderator::kOutput, 0.5f);
    p.setActive(true);

    std::vector<float> inL(block), inR(block), outL(block), outR(block);
    for (int i = 0; i < block; ++i)
    {
        const float x = 0.2f * static_cast<float>(std::sin(2.0 * kPi * 997.0 * i / sr));
        inL[i] = inR[i] = x;
    }
    float* inPtrs[2] = {inL.data(), inR.data()};
    float* outPtrs[2] = {outL.data(), outR.data()};
    AudioBusBuffers inBus {};
    inBus.numChannels = 2;
    inBus.channelBuffers32 = inPtrs;
    AudioBusBuffers outBus {};
    outBus.numChannels = 2;
    outBus.channelBuffers32 = outPtrs;

    ProcessData data {};
    data.processMode = kRealtime;
    data.symbolicSampleSize = kSample32;
    data.numSamples = block;
    data.numInputs = 1;
    data.numOutputs = 1;
    data.inputs = &inBus;
    data.outputs = &outBus;

    for (int i = 0; i < 100; ++i)
        p.process(data);

    std::vector<double> us;
    us.reserve(1200);
    for (int i = 0; i < 1200; ++i)
    {
        const auto t0 = std::chrono::steady_clock::now();
        p.process(data);
        const auto t1 = std::chrono::steady_clock::now();
        us.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
    }
    std::sort(us.begin(), us.end());

    auto pct = [&](double pctl) {
        const size_t idx = static_cast<size_t>(pctl * static_cast<double>(us.size() - 1));
        return us[idx];
    };

    CpuStats stats;
    stats.p95Us = pct(0.95);
    stats.p99Us = pct(0.99);
    stats.maxUs = us.back();
    stats.deadlineUs = 1.0e6 * static_cast<double>(block) / sr;

    p.setActive(false);
    p.terminate();
    return stats;
}


std::vector<float> renderMono(double sr, double seconds, double hz, const Settings& settings, int block = 128)
{
    Processor p;
    if (p.initialize(nullptr) != kResultOk)
        throw std::runtime_error("mono initialize failed");

    SpeakerArrangement monoIn = SpeakerArr::kMono;
    SpeakerArrangement monoOut = SpeakerArr::kMono;
    if (p.setBusArrangements(&monoIn, 1, &monoOut, 1) != kResultOk)
        throw std::runtime_error("mono arrangement failed");

    ProcessSetup setup {};
    setup.processMode = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = block;
    setup.sampleRate = sr;
    if (p.setupProcessing(setup) != kResultOk)
        throw std::runtime_error("mono setup failed");

    p.setTestParameter(Colderator::kCold, settings.cold);
    p.setTestParameter(Colderator::kIce, settings.ice);
    p.setTestParameter(Colderator::kMetal, settings.metal);
    p.setTestParameter(Colderator::kFrost, settings.frost);
    p.setTestParameter(Colderator::kShiver, settings.shiver);
    p.setTestParameter(Colderator::kSpace, settings.space);
    p.setTestParameter(Colderator::kIceMaterial, settings.iceMaterial);
    p.setTestParameter(Colderator::kMetalMaterial, settings.metalMaterial);
    p.setTestParameter(Colderator::kFrostMaterial, settings.frostMaterial);
    p.setTestParameter(Colderator::kShiverMaterial, settings.shiverMaterial);
    p.setTestParameter(Colderator::kSpaceMaterial, settings.spaceMaterial);
    p.setTestParameter(Colderator::kAtmosAType, settings.atmosphereAType);
    p.setTestParameter(Colderator::kAtmosAAmount, settings.atmosphereAAmount);
    p.setTestParameter(Colderator::kAtmosBType, settings.atmosphereBType);
    p.setTestParameter(Colderator::kAtmosBAmount, settings.atmosphereBAmount);
    p.setTestParameter(Colderator::kOutput, 0.5f);
    if (p.setActive(true) != kResultOk)
        throw std::runtime_error("mono active failed");

    const size_t total = static_cast<size_t>(std::llround(sr * seconds));
    std::vector<float> result(total, 0.f);
    std::vector<float> in(block), out(block);
    float* inPtrs[1] = {in.data()};
    float* outPtrs[1] = {out.data()};
    AudioBusBuffers inBus {};
    inBus.numChannels = 1;
    inBus.channelBuffers32 = inPtrs;
    AudioBusBuffers outBus {};
    outBus.numChannels = 1;
    outBus.channelBuffers32 = outPtrs;

    size_t pos = 0;
    while (pos < total)
    {
        const int n = static_cast<int>(std::min<size_t>(block, total - pos));
        for (int i = 0; i < n; ++i)
        {
            const double t = static_cast<double>(pos + static_cast<size_t>(i)) / sr;
            in[i] = 0.2f * static_cast<float>(std::sin(2.0 * kPi * hz * t));
            out[i] = 0.f;
        }

        ProcessData data {};
        data.processMode = kRealtime;
        data.symbolicSampleSize = kSample32;
        data.numSamples = n;
        data.numInputs = 1;
        data.numOutputs = 1;
        data.inputs = &inBus;
        data.outputs = &outBus;
        if (p.process(data) != kResultOk)
            throw std::runtime_error("mono process failed");

        for (int i = 0; i < n; ++i)
            result[pos + static_cast<size_t>(i)] = out[i];
        pos += static_cast<size_t>(n);
    }

    p.setActive(false);
    p.terminate();
    return result;
}

bool invalidParameterValuesAreIgnored(double sr)
{
    Processor a;
    Processor b;
    if (a.initialize(nullptr) != kResultOk || b.initialize(nullptr) != kResultOk)
        return false;

    ProcessSetup setup {};
    setup.processMode = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = 128;
    setup.sampleRate = sr;
    if (a.setupProcessing(setup) != kResultOk || b.setupProcessing(setup) != kResultOk)
        return false;

    a.setTestParameter(Colderator::kCold, 0.42f);
    b.setTestParameter(Colderator::kCold, 0.42f);
    a.setTestParameter(Colderator::kSpace, 0.33f);
    b.setTestParameter(Colderator::kSpace, 0.33f);

    a.setTestParameter(Colderator::kCold, std::numeric_limits<float>::quiet_NaN());
    a.setTestParameter(Colderator::kSpace, std::numeric_limits<float>::infinity());
    a.setTestParameter(Colderator::kMetal, -std::numeric_limits<float>::infinity());

    if (a.setActive(true) != kResultOk || b.setActive(true) != kResultOk)
        return false;

    const auto ra = renderConfiguredProcessor(a, sr, 0.20, 440.0, 128);
    const auto rb = renderConfiguredProcessor(b, sr, 0.20, 440.0, 128);

    a.setActive(false);
    b.setActive(false);
    a.terminate();
    b.terminate();

    return finiteBuffer(ra) && meanAbsDiff(ra, rb, 0) < 1e-8;
}


bool outOfRangeParametersClamp(double sr)
{
    Processor lowA, lowB, highA, highB;
    if (lowA.initialize(nullptr) != kResultOk || lowB.initialize(nullptr) != kResultOk ||
        highA.initialize(nullptr) != kResultOk || highB.initialize(nullptr) != kResultOk)
        return false;

    ProcessSetup setup {};
    setup.processMode = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = 128;
    setup.sampleRate = sr;
    if (lowA.setupProcessing(setup) != kResultOk || lowB.setupProcessing(setup) != kResultOk ||
        highA.setupProcessing(setup) != kResultOk || highB.setupProcessing(setup) != kResultOk)
        return false;

    lowA.setTestParameter(Colderator::kCold, -5.f);
    lowB.setTestParameter(Colderator::kCold, 0.f);
    highA.setTestParameter(Colderator::kCold, 7.f);
    highB.setTestParameter(Colderator::kCold, 1.f);

    if (lowA.setActive(true) != kResultOk || lowB.setActive(true) != kResultOk ||
        highA.setActive(true) != kResultOk || highB.setActive(true) != kResultOk)
        return false;

    const auto la = renderConfiguredProcessor(lowA, sr, 0.12, 440.0, 128);
    const auto lb = renderConfiguredProcessor(lowB, sr, 0.12, 440.0, 128);
    const auto ha = renderConfiguredProcessor(highA, sr, 0.12, 440.0, 128);
    const auto hb = renderConfiguredProcessor(highB, sr, 0.12, 440.0, 128);

    lowA.setActive(false); lowB.setActive(false);
    highA.setActive(false); highB.setActive(false);
    lowA.terminate(); lowB.terminate(); highA.terminate(); highB.terminate();

    return meanAbsDiff(la, lb, 0) < 1e-8 && meanAbsDiff(ha, hb, 0) < 1e-8;
}

bool nullIoIsHandled(double sr)
{
    Processor p;
    if (p.initialize(nullptr) != kResultOk)
        return false;

    ProcessSetup setup {};
    setup.processMode = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = 64;
    setup.sampleRate = sr;
    if (p.setupProcessing(setup) != kResultOk || p.setActive(true) != kResultOk)
        return false;

    AudioBusBuffers inBus {};
    inBus.numChannels = 2;
    inBus.channelBuffers32 = nullptr;
    AudioBusBuffers outBus {};
    outBus.numChannels = 2;
    outBus.channelBuffers32 = nullptr;

    ProcessData data {};
    data.processMode = kRealtime;
    data.symbolicSampleSize = kSample32;
    data.numSamples = 64;
    data.numInputs = 1;
    data.numOutputs = 1;
    data.inputs = &inBus;
    data.outputs = &outBus;

    const bool ok = p.process(data) == kResultOk;
    p.setActive(false);
    p.terminate();
    return ok;
}

bool silentInputPreservesTail(double sr)
{
    constexpr int block = 64;
    Processor p;
    if (p.initialize(nullptr) != kResultOk)
        return false;

    ProcessSetup setup {};
    setup.processMode = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = block;
    setup.sampleRate = sr;
    if (p.setupProcessing(setup) != kResultOk)
        return false;

    p.setTestParameter(Colderator::kCold, 1.f);
    p.setTestParameter(Colderator::kSpace, 1.f);
    p.setTestParameter(Colderator::kOutput, 0.5f);
    if (p.setActive(true) != kResultOk)
        return false;

    std::vector<float> inL(block, 0.f), inR(block, 0.f), outL(block, 0.f), outR(block, 0.f);
    float* inPtrs[2] = {inL.data(), inR.data()};
    float* outPtrs[2] = {outL.data(), outR.data()};
    AudioBusBuffers inBus {};
    inBus.numChannels = 2;
    inBus.channelBuffers32 = inPtrs;
    AudioBusBuffers outBus {};
    outBus.numChannels = 2;
    outBus.channelBuffers32 = outPtrs;

    bool foundTail = false;
    for (int b = 0; b < 80; ++b)
    {
        std::fill(inL.begin(), inL.end(), 0.f);
        std::fill(inR.begin(), inR.end(), 0.f);
        std::fill(outL.begin(), outL.end(), 0.f);
        std::fill(outR.begin(), outR.end(), 0.f);

        if (b == 0)
        {
            inL[0] = 1.f;
            inR[0] = 1.f;
        }

        inBus.silenceFlags = (b == 0) ? 0 : 0x3;

        ProcessData data {};
        data.processMode = kRealtime;
        data.symbolicSampleSize = kSample32;
        data.numSamples = block;
        data.numInputs = 1;
        data.numOutputs = 1;
        data.inputs = &inBus;
        data.outputs = &outBus;

        if (p.process(data) != kResultOk)
            return false;

        if (b > 10)
        {
            for (int i = 0; i < block; ++i)
                if (std::fabs(outL[i]) > 1e-7f || std::fabs(outR[i]) > 1e-7f)
                    foundTail = true;
        }
    }

    p.setActive(false);
    p.terminate();
    return foundTail;
}


struct StereoRender
{
    std::vector<float> left;
    std::vector<float> right;
};

StereoRender renderStereoImpulse(double sr, double seconds, const Settings& settings, int block = 128)
{
    Processor p;
    if (p.initialize(nullptr) != kResultOk)
        throw std::runtime_error("stereo impulse initialize failed");

    ProcessSetup setup {};
    setup.processMode = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = block;
    setup.sampleRate = sr;
    if (p.setupProcessing(setup) != kResultOk)
        throw std::runtime_error("stereo impulse setup failed");

    p.setTestParameter(Colderator::kCold, settings.cold);
    p.setTestParameter(Colderator::kIce, settings.ice);
    p.setTestParameter(Colderator::kMetal, settings.metal);
    p.setTestParameter(Colderator::kFrost, settings.frost);
    p.setTestParameter(Colderator::kShiver, settings.shiver);
    p.setTestParameter(Colderator::kSpace, settings.space);
    p.setTestParameter(Colderator::kIceMaterial, settings.iceMaterial);
    p.setTestParameter(Colderator::kMetalMaterial, settings.metalMaterial);
    p.setTestParameter(Colderator::kFrostMaterial, settings.frostMaterial);
    p.setTestParameter(Colderator::kShiverMaterial, settings.shiverMaterial);
    p.setTestParameter(Colderator::kSpaceMaterial, settings.spaceMaterial);
    p.setTestParameter(Colderator::kAtmosAType, settings.atmosphereAType);
    p.setTestParameter(Colderator::kAtmosAAmount, settings.atmosphereAAmount);
    p.setTestParameter(Colderator::kAtmosBType, settings.atmosphereBType);
    p.setTestParameter(Colderator::kAtmosBAmount, settings.atmosphereBAmount);
    p.setTestParameter(Colderator::kOutput, 0.5f);
    if (p.setActive(true) != kResultOk)
        throw std::runtime_error("stereo impulse active failed");

    const size_t total = static_cast<size_t>(std::llround(sr * seconds));
    StereoRender result;
    result.left.assign(total, 0.f);
    result.right.assign(total, 0.f);

    std::vector<float> inL(block, 0.f), inR(block, 0.f), outL(block, 0.f), outR(block, 0.f);
    float* inPtrs[2] = {inL.data(), inR.data()};
    float* outPtrs[2] = {outL.data(), outR.data()};
    AudioBusBuffers inBus {};
    inBus.numChannels = 2;
    inBus.channelBuffers32 = inPtrs;
    AudioBusBuffers outBus {};
    outBus.numChannels = 2;
    outBus.channelBuffers32 = outPtrs;

    bool sent = false;
    size_t pos = 0;
    while (pos < total)
    {
        const int n = static_cast<int>(std::min<size_t>(block, total - pos));
        std::fill(inL.begin(), inL.end(), 0.f);
        std::fill(inR.begin(), inR.end(), 0.f);
        std::fill(outL.begin(), outL.end(), 0.f);
        std::fill(outR.begin(), outR.end(), 0.f);
        if (!sent)
        {
            inL[0] = 1.f;
            inR[0] = 1.f;
            sent = true;
        }

        ProcessData data {};
        data.processMode = kRealtime;
        data.symbolicSampleSize = kSample32;
        data.numSamples = n;
        data.numInputs = 1;
        data.numOutputs = 1;
        data.inputs = &inBus;
        data.outputs = &outBus;
        if (p.process(data) != kResultOk)
            throw std::runtime_error("stereo impulse process failed");

        for (int i = 0; i < n; ++i)
        {
            result.left[pos + static_cast<size_t>(i)] = outL[i];
            result.right[pos + static_cast<size_t>(i)] = outR[i];
        }
        pos += static_cast<size_t>(n);
    }

    p.setActive(false);
    p.terminate();
    return result;
}

double correlation(const std::vector<float>& a, const std::vector<float>& b, size_t start)
{
    const size_t n = std::min(a.size(), b.size());
    if (n <= start)
        return 1.0;

    double aa = 0.0, bb = 0.0, ab = 0.0;
    for (size_t i = start; i < n; ++i)
    {
        const double x = a[i];
        const double y = b[i];
        aa += x * x;
        bb += y * y;
        ab += x * y;
    }
    const double denom = std::sqrt(aa * bb);
    return denom > 1e-20 ? ab / denom : 1.0;
}

double spectralMagnitude(const std::vector<float>& x, double sr, double hz, size_t start, size_t end)
{
    end = std::min(end, x.size());
    if (end <= start)
        return 0.0;

    double re = 0.0;
    double im = 0.0;
    for (size_t i = start; i < end; ++i)
    {
        const double phase = 2.0 * kPi * hz * static_cast<double>(i) / sr;
        re += static_cast<double>(x[i]) * std::cos(phase);
        im -= static_cast<double>(x[i]) * std::sin(phase);
    }
    return std::sqrt(re * re + im * im) / static_cast<double>(end - start);
}

double tailEnergy(const std::vector<float>& x, size_t start)
{
    if (x.size() <= start)
        return 0.0;

    double e = 0.0;
    for (size_t i = start; i < x.size(); ++i)
        e += static_cast<double>(x[i]) * static_cast<double>(x[i]);
    return e;
}

bool finiteBuffer(const std::vector<float>& x)
{
    for (float v : x)
        if (!std::isfinite(v))
            return false;
    return true;
}

double meanAbsDiff(const std::vector<float>& a, const std::vector<float>& b, size_t skip)
{
    const size_t n = std::min(a.size(), b.size());
    if (n <= skip)
        return 0.0;

    double sum = 0.0;
    for (size_t i = skip; i < n; ++i)
        sum += std::fabs(static_cast<double>(a[i]) - static_cast<double>(b[i]));
    return sum / static_cast<double>(n - skip);
}

double maxAbs(const std::vector<float>& x)
{
    double m = 0.0;
    for (float v : x)
        m = std::max(m, std::fabs(static_cast<double>(v)));
    return m;
}

double toneAmplitude(const std::vector<float>& x, double sr, double hz, size_t skip)
{
    if (x.size() <= skip)
        return 0.0;

    double re = 0.0;
    double im = 0.0;
    const size_t n = x.size() - skip;

    for (size_t i = skip; i < x.size(); ++i)
    {
        const double phase = 2.0 * kPi * hz * static_cast<double>(i) / sr;
        re += static_cast<double>(x[i]) * std::cos(phase);
        im -= static_cast<double>(x[i]) * std::sin(phase);
    }

    return 2.0 * std::sqrt(re * re + im * im) / static_cast<double>(n);
}

void require(bool condition, const std::string& message, int& failures)
{
    if (condition)
        std::cout << "[PASS] " << message << "\n";
    else
    {
        std::cout << "[FAIL] " << message << "\n";
        ++failures;
    }
}

} // namespace

int main()
{
    int failures = 0;

    try
    {
        {
            Processor p;
            require(p.initialize(nullptr) == kResultOk, "processor initializes for bus-layout test", failures);
            SpeakerArrangement monoIn = SpeakerArr::kMono;
            SpeakerArrangement monoOut = SpeakerArr::kMono;
            SpeakerArrangement stereoIn = SpeakerArr::kStereo;
            SpeakerArrangement stereoOut = SpeakerArr::kStereo;
            require(p.setBusArrangements(&monoIn, 1, &monoOut, 1) == kResultOk,
                    "matched mono bus layout is accepted", failures);
            require(p.setBusArrangements(&stereoIn, 1, &stereoOut, 1) == kResultOk,
                    "matched stereo bus layout is accepted", failures);
            p.terminate();
        }

        require(lifecycleAndRateChangeStable(),
                "activate/deactivate and sample-rate changes remain stable", failures);

        {
            const auto cpu64 = measureCpu(48000.0, 64);
            const auto cpu256 = measureCpu(48000.0, 256);
            std::cout << "[INFO] CPU 48k/64 p95=" << cpu64.p95Us
                      << "us p99=" << cpu64.p99Us << "us max=" << cpu64.maxUs
                      << "us deadline=" << cpu64.deadlineUs << "us\n";
            std::cout << "[INFO] CPU 48k/256 p95=" << cpu256.p95Us
                      << "us p99=" << cpu256.p99Us << "us max=" << cpu256.maxUs
                      << "us deadline=" << cpu256.deadlineUs << "us\n";
            require(cpu64.p99Us < cpu64.deadlineUs && cpu256.p99Us < cpu256.deadlineUs,
                    "CPU p99 stays inside realtime block deadlines on CI", failures);
        }

        require(nullIoIsHandled(48000.0),
                "null audio buffer pointers are handled without crash", failures);

        for (double sr : {44100.0, 48000.0, 96000.0})
        {
            require(stateRoundtripMatches(sr),
                    "component state roundtrip reproduces DSP behavior at " +
                    std::to_string(static_cast<int>(sr)) + " Hz",
                    failures);

            {
                Processor tailProbe;
                require(tailProbe.initialize(nullptr) == kResultOk,
                        "tail probe initializes", failures);
                ProcessSetup tailSetup {};
                tailSetup.processMode = kRealtime;
                tailSetup.symbolicSampleSize = kSample32;
                tailSetup.maxSamplesPerBlock = 128;
                tailSetup.sampleRate = sr;
                require(tailProbe.setupProcessing(tailSetup) == kResultOk,
                        "tail probe setup succeeds", failures);
                const uint32 reportedTail = tailProbe.getTailSamples();
                require(reportedTail >= static_cast<uint32>(sr * 11.5) &&
                        reportedTail <= static_cast<uint32>(sr * 12.5),
                        "reported tail covers long cinematic Space decay", failures);
                tailProbe.terminate();
            }
            const Settings paritySettings {0.78f, 0.52f, 0.63f, 0.35f, 0.28f, 0.67f};
            const auto realtimeParity = renderMode(sr, kRealtime, paritySettings, 128);
            const auto offlineParity = renderMode(sr, kOffline, paritySettings, 128);
            require(meanAbsDiff(realtimeParity, offlineParity, 0) < 1e-8,
                    "offline and realtime processing are sample-identical", failures);

            require(activeStateLoadStable(sr),
                    "state load while active remains finite and takes effect", failures);

            require(invalidParameterValuesAreIgnored(sr),
                    "NaN and Inf parameter values are ignored safely", failures);

            require(outOfRangeParametersClamp(sr),
                    "out-of-range normalized parameter values clamp safely", failures);

            require(silentInputPreservesTail(sr),
                    "silent-input flags do not truncate an active SPACE tail", failures);

            const Settings monoSettings {0.62f, 0.38f, 0.44f, 0.21f, 0.18f, 0.35f};
            const auto stereoForMono = renderSine(sr, 0.35, 440.0, monoSettings, 128);
            const auto monoRender = renderMono(sr, 0.35, 440.0, monoSettings, 128);
            require(meanAbsDiff(stereoForMono, monoRender, 0) < 1e-8,
                    "mono DSP matches stereo left-channel behavior", failures);

            const size_t skip = static_cast<size_t>(sr * 0.15);

            const auto dry = renderSine(sr, 0.8, 440.0, {});
            const auto coldOffModules = renderSine(sr, 0.8, 440.0, {1.f, 0.f, 0.f, 0.f, 0.f, 0.f});
            const auto modulesWithColdOff = renderSine(sr, 0.8, 440.0, {0.f, 1.f, 1.f, 1.f, 1.f, 1.f});

            const auto cold25 = renderSine(sr, 0.8, 440.0, {0.25f, 1.f, 1.f, 1.f, 1.f, 1.f});
            const auto cold50 = renderSine(sr, 0.8, 440.0, {0.50f, 1.f, 1.f, 1.f, 1.f, 1.f});
            const auto cold75Stage = renderSine(sr, 0.8, 440.0, {0.75f, 1.f, 1.f, 1.f, 1.f, 1.f});
            const auto cold90 = renderSine(sr, 0.8, 440.0, {0.90f, 1.f, 1.f, 1.f, 1.f, 1.f});
            const auto cold100 = renderSine(sr, 0.8, 440.0, {1.f, 1.f, 1.f, 1.f, 1.f, 1.f});

            const auto ice25 = renderSine(sr, 0.8, 440.0, {1.f, 0.25f, 0.f, 0.f, 0.f, 0.f});
            const auto ice50 = renderSine(sr, 0.8, 440.0, {1.f, 0.50f, 0.f, 0.f, 0.f, 0.f});
            const auto ice100 = renderSine(sr, 0.8, 440.0, {1.f, 1.f, 0.f, 0.f, 0.f, 0.f});
            const auto metal25 = renderSine(sr, 0.8, 440.0, {1.f, 0.f, 0.25f, 0.f, 0.f, 0.f});
            const auto metal50 = renderSine(sr, 0.8, 440.0, {1.f, 0.f, 0.50f, 0.f, 0.f, 0.f});
            const auto metal75 = renderSine(sr, 0.8, 440.0, {1.f, 0.f, 0.75f, 0.f, 0.f, 0.f});
            const auto metal100 = renderSine(sr, 0.8, 440.0, {1.f, 0.f, 1.f, 0.f, 0.f, 0.f});
            const auto frost25 = renderSine(sr, 0.8, 440.0, {1.f, 0.f, 0.f, 0.25f, 0.f, 0.f});
            const auto frost50 = renderSine(sr, 0.8, 440.0, {1.f, 0.f, 0.f, 0.50f, 0.f, 0.f});
            const auto frost100 = renderSine(sr, 0.8, 440.0, {1.f, 0.f, 0.f, 1.f, 0.f, 0.f});
            const auto frost100Repeat = renderSine(sr, 0.8, 440.0, {1.f, 0.f, 0.f, 1.f, 0.f, 0.f});
            const auto frostSilence = renderSine(sr, 0.8, 0.0, {1.f, 0.f, 0.f, 1.f, 0.f, 0.f});
            const auto textureSilence = renderSine(sr, 0.8, 0.0, {1.f, 1.f, 1.f, 1.f, 1.f, 0.f});
            const auto textureRepeatA = renderSine(sr, 0.8, 440.0, {1.f, 0.72f, 0.66f, 0.80f, 0.64f, 0.f});
            const auto textureRepeatB = renderSine(sr, 0.8, 440.0, {1.f, 0.72f, 0.66f, 0.80f, 0.64f, 0.f});
            const auto shiver25 = renderSine(sr, 0.8, 440.0, {1.f, 0.f, 0.f, 0.f, 0.25f, 0.f});
            const auto shiver50 = renderSine(sr, 0.8, 440.0, {1.f, 0.f, 0.f, 0.f, 0.50f, 0.f});
            const auto shiver100 = renderSine(sr, 0.8, 440.0, {1.f, 0.f, 0.f, 0.f, 1.f, 0.f});
            const auto shiverLowDry = renderSine(sr, 0.8, 80.0, {});
            const auto coldOnly80 = renderSine(sr, 0.8, 80.0, {1.f, 0.f, 0.f, 0.f, 0.f, 0.f});
            const auto shiverOnlyNoCold80 = renderSine(sr, 0.8, 80.0, {0.f, 0.f, 0.f, 0.f, 1.f, 0.f});
            const auto shiverLowFx = renderSine(sr, 0.8, 80.0, {1.f, 0.f, 0.f, 0.f, 1.f, 0.f});
            const auto space50 = renderSine(sr, 0.8, 440.0, {1.f, 0.f, 0.f, 0.f, 0.f, 0.50f});
            const auto space75 = renderSine(sr, 0.8, 440.0, {1.f, 0.f, 0.f, 0.f, 0.f, 0.75f});
            const auto space100 = renderSine(sr, 0.8, 440.0, {1.f, 0.f, 0.f, 0.f, 0.f, 1.f});

            // Per-module spike fingerprints: one full-scale impulse through COLD=100
            // with exactly one participating module. These expose the actual
            // time-domain signature far more clearly than steady tones.
            const auto impulseDry = renderImpulse(sr, 0.40, {});
            const auto impulseIce50 = renderImpulse(sr, 0.40, {1.f, 0.50f, 0.f, 0.f, 0.f, 0.f});
            const auto impulseIce100 = renderImpulse(sr, 0.40, {1.f, 1.f, 0.f, 0.f, 0.f, 0.f});
            const auto impulseMetal50 = renderImpulse(sr, 0.40, {1.f, 0.f, 0.50f, 0.f, 0.f, 0.f});
            const auto impulseMetal100 = renderImpulse(sr, 0.40, {1.f, 0.f, 1.f, 0.f, 0.f, 0.f});
            const auto impulseFrost100 = renderImpulse(sr, 0.40, {1.f, 0.f, 0.f, 1.f, 0.f, 0.f});
            const auto impulseShiver100 = renderImpulse(sr, 0.40, {1.f, 0.f, 0.f, 0.f, 1.f, 0.f});
            const auto impulseSpace50 = renderImpulse(sr, 4.0, {1.f, 0.f, 0.f, 0.f, 0.f, 0.50f});
            const auto impulseSpace100 = renderImpulse(sr, 4.0, {1.f, 0.f, 0.f, 0.f, 0.f, 1.f});
            const auto impulseCold100 = renderImpulse(sr, 4.0, {1.f, 1.f, 1.f, 1.f, 1.f, 1.f});
            Settings bloomImpulseSettings {};
            bloomImpulseSettings.cold = 1.f;
            bloomImpulseSettings.atmosphereAType = 8.f / 9.f;
            bloomImpulseSettings.atmosphereAAmount = 1.f;
            const auto impulseFrozenBloom =
                renderImpulse(sr, 2.0, bloomImpulseSettings, 128);
            const auto impulseCinematicLow = renderImpulse(sr, 1.2, {0.30f, 1.f, 0.f, 0.f, 0.f, 0.f});
            const auto impulseCinematicHigh = renderImpulse(sr, 1.2, {0.92f, 1.f, 0.f, 0.f, 0.f, 0.f});

            require(finiteBuffer(dry) && finiteBuffer(cold100) &&
                    finiteBuffer(ice100) && finiteBuffer(metal100) &&
                    finiteBuffer(frost100) && finiteBuffer(shiver100) &&
                    finiteBuffer(space100),
                    "finite output at " + std::to_string(static_cast<int>(sr)) + " Hz",
                    failures);

            require(meanAbsDiff(dry, renderSine(sr, 0.8, 440.0, {}), skip) < 1e-8,
                    "neutral render deterministic at " + std::to_string(static_cast<int>(sr)) + " Hz",
                    failures);
            require(meanAbsDiff(dry, coldOffModules, skip) < 1e-8,
                    "COLD 100% with every module at 0% is truly neutral", failures);
            require(meanAbsDiff(dry, modulesWithColdOff, skip) < 1e-8,
                    "all modules at 100% with COLD at 0% are truly neutral", failures);

            if (static_cast<int>(sr) == 48000)
            {
                require(Colderator::FrozenSources::k_storm_wind_count >
                            static_cast<std::size_t>(Colderator::FrozenSources::kSampleRate * 5),
                        "CI build embeds a real Frozen Storm source segment", failures);
                require(Colderator::FrozenSources::k_ice_crackle_count >
                            static_cast<std::size_t>(Colderator::FrozenSources::kSampleRate),
                        "CI build embeds a real Ice Crack source segment", failures);
                require(Colderator::FrozenSources::k_cold_metal_air_count >
                            static_cast<std::size_t>(Colderator::FrozenSources::kSampleRate * 3),
                        "CI build embeds a real industrial metal-air segment", failures);
                require(Colderator::FrozenSources::k_metal_chime_count >
                            static_cast<std::size_t>(Colderator::FrozenSources::kSampleRate * 2),
                        "CI build embeds a real metallic-event segment", failures);
                auto materialValue = [](int index) {
                    return static_cast<float>(index) /
                           static_cast<float>(Colderator::kMaterialCount - 1);
                };

                auto requireSixDistinct = [&](const std::string& label, auto makeSettings,
                                              double threshold) {
                    std::vector<float> previous;
                    for (int m = 0; m < Colderator::kMaterialCount; ++m)
                    {
                        const auto rendered =
                            renderSine(sr, 0.45, 440.0, makeSettings(materialValue(m)), 128);
                        if (!previous.empty())
                        {
                            require(meanAbsDiff(previous, rendered, skip) > threshold,
                                    label + " material " + std::to_string(m + 1) +
                                    " has a distinct DSP fingerprint", failures);
                        }
                        previous = rendered;
                    }
                };

                requireSixDistinct("ICE", [](float m) {
                    Settings v {1.f, 0.78f, 0.f, 0.f, 0.f, 0.f};
                    v.iceMaterial = m;
                    return v;
                }, 1e-4);

                requireSixDistinct("METAL", [](float m) {
                    Settings v {1.f, 0.f, 0.78f, 0.f, 0.f, 0.f};
                    v.metalMaterial = m;
                    return v;
                }, 1e-4);

                Settings metalCinematic {};
                metalCinematic.cold = 1.f;
                metalCinematic.metal = 0.50f;
                metalCinematic.metalMaterial = 4.f / 5.f; // Machine
                const auto metalMachine50 =
                    renderSine(sr, 1.5, 110.0, metalCinematic, 128);
                require(meanAbsDiff(dry, metalMachine50, skip) > 5e-3,
                        "METAL Machine 50% creates an obvious cinematic material transformation", failures);
                require(maxAbs(metalMachine50) < 8.0,
                        "METAL Machine 50% remains bounded", failures);

                requireSixDistinct("FROST", [](float m) {
                    Settings v {1.f, 0.f, 0.f, 0.78f, 0.f, 0.f};
                    v.frostMaterial = m;
                    return v;
                }, 5e-5);

                Settings frostLow {};
                frostLow.cold = 0.25f;
                frostLow.frost = 0.50f;
                frostLow.frostMaterial = 0.f;
                const auto frostLowRender =
                    renderSine(sr, 1.2, 220.0, frostLow, 128);
                const double frostLowFund =
                    toneAmplitude(frostLowRender, sr, 220.0, skip);
                const double dry220 =
                    toneAmplitude(renderSine(sr, 1.2, 220.0, {}, 128), sr, 220.0, skip);
                require(dry220 > 1e-12 && frostLowFund / dry220 > 0.45,
                        "FROST low-range remains clearly playable and pitch-traceable", failures);

                Settings frost50Scene {};
                frost50Scene.cold = 1.f;
                frost50Scene.frost = 0.50f;
                frost50Scene.frostMaterial = 5.f / 5.f; // Deep Freeze
                const auto frost50SceneRender =
                    renderSine(sr, 1.5, 220.0, frost50Scene, 128);
                require(meanAbsDiff(renderSine(sr, 1.5, 220.0, {}, 128),
                                    frost50SceneRender, skip) > 4e-3,
                        "FROST 50% creates an obvious frozen-surface transformation", failures);
                require(maxAbs(frost50SceneRender) < 8.0,
                        "FROST Deep Freeze 50% remains bounded", failures);

                requireSixDistinct("SHIVER", [](float m) {
                    Settings v {1.f, 0.f, 0.f, 0.f, 0.78f, 0.f};
                    v.shiverMaterial = m;
                    return v;
                }, 5e-5);

                Settings shiverPlayable {};
                shiverPlayable.cold = 0.25f;
                shiverPlayable.shiver = 0.50f;
                shiverPlayable.shiverMaterial = 0.f; // Tremble
                const auto shiverPlayableRender =
                    renderSine(sr, 1.2, 220.0, shiverPlayable, 128);
                const auto dry220Shiver =
                    renderSine(sr, 1.2, 220.0, {}, 128);
                const double dry220ShiverFund =
                    toneAmplitude(dry220Shiver, sr, 220.0, skip);
                const double shiver220Fund =
                    toneAmplitude(shiverPlayableRender, sr, 220.0, skip);
                require(dry220ShiverFund > 1e-12 &&
                        shiver220Fund / dry220ShiverFund > 0.55,
                        "SHIVER low-range remains clearly playable and pitch-traceable", failures);

                Settings shiverSpasm {};
                shiverSpasm.cold = 1.f;
                shiverSpasm.shiver = 0.50f;
                shiverSpasm.shiverMaterial = 4.f / 5.f; // Spasm
                const auto shiverSpasm50 =
                    renderSine(sr, 1.5, 220.0, shiverSpasm, 128);
                require(meanAbsDiff(dry220Shiver, shiverSpasm50, skip) > 3e-3,
                        "SHIVER Spasm 50% creates an obvious physical cold-motion identity", failures);
                require(maxAbs(shiverSpasm50) < 8.0,
                        "SHIVER Spasm 50% remains bounded", failures);

                requireSixDistinct("SPACE", [](float m) {
                    Settings v {1.f, 0.f, 0.f, 0.f, 0.f, 0.78f};
                    v.spaceMaterial = m;
                    return v;
                }, 5e-5);

                // Atmosphere types must be real generators, not labels.
                std::vector<float> previousAtmos;
                for (int a = 0; a < Colderator::kAtmosphereTypeCount; ++a)
                {
                    Settings v {};
                    v.cold = 1.f;
                    v.atmosphereAType =
                        static_cast<float>(a) /
                        static_cast<float>(Colderator::kAtmosphereTypeCount - 1);
                    v.atmosphereAAmount = 0.80f;
                    const auto rendered = renderSine(sr, 1.0, 110.0, v, 128);
                    require(finiteBuffer(rendered),
                            "Atmosphere type " + std::to_string(a + 1) +
                            " remains finite", failures);
                    if (!previousAtmos.empty())
                    {
                        require(meanAbsDiff(previousAtmos, rendered, skip) > 2e-5,
                                "Atmosphere type " + std::to_string(a + 1) +
                                " has a distinct generator fingerprint", failures);
                    }
                    previousAtmos = rendered;
                }

                Settings wind50 {};
                wind50.cold = 1.f;
                wind50.atmosphereAType = 0.f / 9.f; // Wind
                wind50.atmosphereAAmount = 0.50f;
                const auto wind50Render = renderSine(sr, 1.5, 110.0, wind50, 128);

                Settings storm50 {};
                storm50.cold = 1.f;
                storm50.atmosphereAType = 1.f / 9.f; // Frozen Storm
                storm50.atmosphereAAmount = 0.50f;
                const auto storm50Render = renderSine(sr, 1.5, 110.0, storm50, 128);

                require(meanAbsDiff(wind50Render, storm50Render, skip) > 2e-3,
                        "Frozen Storm 50% has a clearly distinct cinematic identity from Wind", failures);
                require(maxAbs(storm50Render) < 8.0,
                        "Frozen Storm 50% remains bounded", failures);

                Settings air50 {};
                air50.cold = 1.f;
                air50.atmosphereAType = 6.f / 9.f; // Air
                air50.atmosphereAAmount = 0.50f;
                const auto air50Render =
                    renderSine(sr, 1.5, 110.0, air50, 128);

                require(meanAbsDiff(wind50Render, air50Render, skip) > 2e-3,
                        "Air 50% has a distinct thin-cold identity from Wind", failures);
                require(maxAbs(air50Render) < 8.0,
                        "Air 50% remains bounded", failures);

                Settings rumble50 {};
                rumble50.cold = 1.f;
                rumble50.atmosphereAType = 3.f / 9.f; // Rumble
                rumble50.atmosphereAAmount = 0.50f;
                const auto rumble50Render =
                    renderSine(sr, 1.5, 110.0, rumble50, 128);

                require(meanAbsDiff(drone50Render, rumble50Render, skip) > 2e-3,
                        "Rumble 50% has a distinct structural identity from Drone", failures);
                require(maxAbs(rumble50Render) < 8.0,
                        "Rumble 50% remains bounded", failures);

                Settings rumbleSilence {};
                rumbleSilence.cold = 1.f;
                rumbleSilence.atmosphereAType = 3.f / 9.f;
                rumbleSilence.atmosphereAAmount = 1.f;
                const auto rumbleSilenceRender =
                    renderSine(sr, 1.2, 0.0, rumbleSilence, 128);
                double rumbleSilencePeak = 0.0;
                for (float v : rumbleSilenceRender)
                    rumbleSilencePeak = std::max(
                        rumbleSilencePeak, std::fabs(static_cast<double>(v)));
                require(rumbleSilencePeak < 1e-12,
                        "Rumble produces no autonomous output on fresh silence", failures);

                Settings airSilence {};
                airSilence.cold = 1.f;
                airSilence.atmosphereAType = 6.f / 9.f;
                airSilence.atmosphereAAmount = 1.f;
                const auto airSilenceRender =
                    renderSine(sr, 1.2, 0.0, airSilence, 128);
                double airSilencePeak = 0.0;
                for (float v : airSilenceRender)
                    airSilencePeak = std::max(
                        airSilencePeak, std::fabs(static_cast<double>(v)));
                require(airSilencePeak < 1e-12,
                        "Air produces no autonomous output on fresh silence", failures);

                Settings landscape50 {};
                landscape50.cold = 1.f;
                landscape50.atmosphereAType = 7.f / 9.f; // Frozen Landscape
                landscape50.atmosphereAAmount = 0.50f;
                const auto landscape50Render =
                    renderSine(sr, 1.8, 110.0, landscape50, 128);

                require(meanAbsDiff(storm50Render, landscape50Render, skip) > 2e-3,
                        "Frozen Landscape 50% has a distinct scene identity from Storm", failures);
                require(maxAbs(landscape50Render) < 8.0,
                        "Frozen Landscape 50% remains bounded", failures);

                Settings landscapeSilence {};
                landscapeSilence.cold = 1.f;
                landscapeSilence.atmosphereAType = 7.f / 9.f;
                landscapeSilence.atmosphereAAmount = 1.f;
                const auto landscapeSilenceRender =
                    renderSine(sr, 1.2, 0.0, landscapeSilence, 128);
                double landscapeSilencePeak = 0.0;
                for (float v : landscapeSilenceRender)
                    landscapeSilencePeak = std::max(
                        landscapeSilencePeak, std::fabs(static_cast<double>(v)));
                require(landscapeSilencePeak < 1e-12,
                        "Frozen Landscape produces no autonomous output on silence", failures);

                Settings bloom50 {};
                bloom50.cold = 1.f;
                bloom50.atmosphereAType = 8.f / 9.f; // Frozen Bloom
                bloom50.atmosphereAAmount = 0.50f;
                const auto bloom50Render =
                    renderSine(sr, 1.8, 220.0, bloom50, 128);

                Settings drone50 {};
                drone50.cold = 1.f;
                drone50.atmosphereAType = 2.f / 9.f; // Drone
                drone50.atmosphereAAmount = 0.50f;
                const auto drone50Render =
                    renderSine(sr, 1.8, 220.0, drone50, 128);

                require(meanAbsDiff(drone50Render, bloom50Render, skip) > 2e-3,
                        "Frozen Bloom 50% has a distinct identity from Drone", failures);
                require(maxAbs(bloom50Render) < 8.0,
                        "Frozen Bloom 50% remains bounded", failures);

                Settings bloomSilence {};
                bloomSilence.cold = 1.f;
                bloomSilence.atmosphereAType = 8.f / 9.f;
                bloomSilence.atmosphereAAmount = 1.f;
                const auto bloomSilenceRender =
                    renderSine(sr, 1.2, 0.0, bloomSilence, 128);
                double bloomSilencePeak = 0.0;
                for (float v : bloomSilenceRender)
                    bloomSilencePeak = std::max(
                        bloomSilencePeak, std::fabs(static_cast<double>(v)));
                require(bloomSilencePeak < 1e-12,
                        "Frozen Bloom produces no autonomous output on fresh silence", failures);


                Settings machine50 {};
                machine50.cold = 1.f;
                machine50.atmosphereAType = 9.f / 9.f; // Machine
                machine50.atmosphereAAmount = 0.50f;
                const auto machine50Render =
                    renderSine(sr, 1.8, 110.0, machine50, 128);

                Settings distantMetal50 {};
                distantMetal50.cold = 1.f;
                distantMetal50.atmosphereAType = 4.f / 9.f; // Distant Metal
                distantMetal50.atmosphereAAmount = 0.50f;
                const auto distantMetal50Render =
                    renderSine(sr, 1.8, 110.0, distantMetal50, 128);

                require(maxAbs(distantMetal50Render) < 8.0,
                        "Distant Metal 50% remains bounded", failures);

                Settings distantMetalSilence {};
                distantMetalSilence.cold = 1.f;
                distantMetalSilence.atmosphereAType = 4.f / 9.f;
                distantMetalSilence.atmosphereAAmount = 1.f;
                const auto distantMetalSilenceRender =
                    renderSine(sr, 1.2, 0.0, distantMetalSilence, 128);
                double distantMetalSilencePeak = 0.0;
                for (float v : distantMetalSilenceRender)
                    distantMetalSilencePeak = std::max(
                        distantMetalSilencePeak, std::fabs(static_cast<double>(v)));
                require(distantMetalSilencePeak < 1e-12,
                        "Distant Metal produces no autonomous output on fresh silence", failures);

                require(meanAbsDiff(distantMetal50Render, machine50Render, skip) > 2e-3,
                        "Machine 50% has a distinct mechanical-room identity from Distant Metal", failures);
                require(meanAbsDiff(landscape50Render, machine50Render, skip) > 2e-3,
                        "Machine 50% has a distinct identity from Frozen Landscape", failures);
                require(maxAbs(machine50Render) < 8.0,
                        "Machine 50% remains bounded", failures);

                Settings machineSilence {};
                machineSilence.cold = 1.f;
                machineSilence.atmosphereAType = 9.f / 9.f;
                machineSilence.atmosphereAAmount = 1.f;
                const auto machineSilenceRender =
                    renderSine(sr, 1.2, 0.0, machineSilence, 128);
                double machineSilencePeak = 0.0;
                for (float v : machineSilenceRender)
                    machineSilencePeak = std::max(
                        machineSilencePeak, std::fabs(static_cast<double>(v)));
                require(machineSilencePeak < 1e-12,
                        "Machine produces no autonomous output on fresh silence", failures);

                Settings dualAtmos {};
                dualAtmos.cold = 1.f;
                dualAtmos.atmosphereAType = 2.f / 9.f; // Drone
                dualAtmos.atmosphereAAmount = 0.70f;
                dualAtmos.atmosphereBType = 1.f / 9.f; // Storm
                dualAtmos.atmosphereBAmount = 0.70f;
                const auto droneStorm = renderSine(sr, 1.2, 110.0, dualAtmos, 128);

                Settings droneOnly = dualAtmos;
                droneOnly.atmosphereBAmount = 0.f;
                const auto drone = renderSine(sr, 1.2, 110.0, droneOnly, 128);
                require(meanAbsDiff(droneStorm, drone, skip) > 1e-4,
                        "dual atmosphere slots combine into a new scene", failures);
            }

            const auto automation64 = renderAutomationPattern(sr, 64);
            const auto automation257 = renderAutomationPattern(sr, 257);
            require(meanAbsDiff(automation64, automation257, 0) < 2e-4,
                    "sample-offset automation is block-boundary independent", failures);

            const auto bypassAutomation = renderSingleBlockBypassAutomation(sr);
            double bypassExactDiff = 0.0;
            for (size_t i = 32; i < 96; ++i)
                bypassExactDiff = std::max(bypassExactDiff,
                    std::fabs(static_cast<double>(bypassAutomation[i] - bypassAutomation[128 + i])));
            require(bypassExactDiff < 1e-7,
                    "multiple bypass points inside one block apply at exact sample offsets", failures);

            const auto block32 = renderSine(sr, 0.8, 440.0, {0.72f, 0.35f, 0.40f, 0.25f, 0.20f, 0.30f}, 32);
            const auto block128 = renderSine(sr, 0.8, 440.0, {0.72f, 0.35f, 0.40f, 0.25f, 0.20f, 0.30f}, 128);
            const auto block1024 = renderSine(sr, 0.8, 440.0, {0.72f, 0.35f, 0.40f, 0.25f, 0.20f, 0.30f}, 1024);
            require(meanAbsDiff(block32, block128, skip) < 2e-4 &&
                    meanAbsDiff(block128, block1024, skip) < 2e-4,
                    "block-size response remains consistent at " + std::to_string(static_cast<int>(sr)) + " Hz",
                    failures);

            const auto denormStress = renderImpulse(sr, 4.0, {1.f, 1.f, 1.f, 1.f, 1.f, 1.f}, 64);
            require(finiteBuffer(denormStress), "denormal stress remains finite", failures);
            require(maxAbs(denormStress) < 20.0, "denormal stress remains bounded", failures);
            require(maxAbs(cold100) < 8.0 && maxAbs(metal100) < 8.0 &&
                    maxAbs(frost100) < 8.0,
                    "Frozen Core extreme settings remain bounded", failures);

            const int bypassBlockSize = 64;
            const int totalBlocks = 700;
            const int bypassStartBlock = 120;
            const int bypassEndBlock = 420;
            const Settings bypassSettings {0.75f, 0.55f, 0.65f, 0.25f, 0.20f, 0.85f};
            const auto bypassed = renderImpulseWithBypassWindow(sr, bypassBlockSize, totalBlocks,
                                                                bypassStartBlock, bypassEndBlock,
                                                                bypassSettings);
            const auto continuous = renderImpulseWithBypassWindow(sr, bypassBlockSize, totalBlocks,
                                                                  totalBlocks + 1, totalBlocks + 2,
                                                                  bypassSettings);
            const size_t bypassStartSample = static_cast<size_t>(bypassStartBlock * bypassBlockSize);
            const size_t bypassEndSample = static_cast<size_t>(bypassEndBlock * bypassBlockSize);

            double bypassWindowPeak = 0.0;
            for (size_t i = bypassStartSample; i < bypassEndSample; ++i)
                bypassWindowPeak = std::max(bypassWindowPeak, std::fabs(static_cast<double>(bypassed[i])));
            require(bypassWindowPeak < 1e-12,
                    "bypass outputs exact dry silence while DSP state advances", failures);

            require(meanAbsDiff(bypassed, continuous, bypassEndSample) < 1e-6,
                    "bypass state resumes without stale tail discontinuity", failures);

            const double dCold25 = meanAbsDiff(dry, cold25, skip);
            const double dCold50 = meanAbsDiff(dry, cold50, skip);
            const double dCold75 = meanAbsDiff(dry, cold75Stage, skip);
            const double dCold90 = meanAbsDiff(dry, cold90, skip);
            const double dCold100 = meanAbsDiff(dry, cold100, skip);
            const double dIce25 = meanAbsDiff(dry, ice25, skip);
            const double dIce50 = meanAbsDiff(dry, ice50, skip);
            const double dIce100 = meanAbsDiff(dry, ice100, skip);
            const double dMetal25 = meanAbsDiff(dry, metal25, skip);
            const double dMetal50 = meanAbsDiff(dry, metal50, skip);
            const double dMetal75 = meanAbsDiff(dry, metal75, skip);
            const double dMetal100 = meanAbsDiff(dry, metal100, skip);
            const double dFrost25 = meanAbsDiff(dry, frost25, skip);
            const double dFrost50 = meanAbsDiff(dry, frost50, skip);
            const double dFrost100 = meanAbsDiff(dry, frost100, skip);
            const double dShiver25 = meanAbsDiff(dry, shiver25, skip);
            const double dShiver50 = meanAbsDiff(dry, shiver50, skip);
            const double dShiver100 = meanAbsDiff(dry, shiver100, skip);
            const double dSpace50 = meanAbsDiff(dry, space50, skip);
            const double dSpace100 = meanAbsDiff(dry, space100, skip);

            require(dCold25 > 1e-5, "COLD 25% is already measurably cool", failures);
            require(dCold50 > 8e-3,
                    "COLD 50% is already a strong signature transformation", failures);
            require(dCold75 > 8e-3,
                    "COLD 75% remains a strong transformed state", failures);
            // Do not require monotonic dry-distance at every upper macro point:
            // material morphs can rotate into a different texture rather than
            // simply moving farther from dry. 50% and 100% carry the strong
            // working-region / extreme-region requirements.
            require(dCold100 > dCold50 * 1.12,
                    "COLD 100% remains a meaningful extreme transformation", failures);
            require(dCold50 > 8e-3, "COLD 50% is a strong signature transformation", failures);
            const double ice25to50Delta =
                meanAbsDiff(ice25, ice50, skip);
            std::cout << "[INFO] ICE 25to50 SR=" << static_cast<int>(sr)
                      << " delta=" << ice25to50Delta << "\n";
            require(ice25to50Delta > 2e-3,
                    "ICE 50% is clearly distinct from the 25% crystal/glass state", failures);
            require(dMetal50 > 5e-3 && dMetal50 > dMetal25 * 1.35,
                    "METAL 50% is a dominant industrial transformation", failures);
            require(dMetal75 > dMetal50 * 1.10,
                    "METAL 75% advances clearly beyond the normal-range setting", failures);

            const double metalSbA = toneAmplitude(metal75, sr, 693.0, skip);
            const double metalSbB = toneAmplitude(metal75, sr, 1573.0, skip);
            require(metalSbA + metalSbB > 5e-4,
                    "METAL 75% creates deliberate inharmonic FM/ringmod sidebands", failures);
            require(dFrost50 > 5e-3 && dFrost50 > dFrost25 * 1.35,
                    "FROST 50% forms a dominant frozen surface texture", failures);
            std::cout << "[INFO] FROST EXTREME SR=" << static_cast<int>(sr)
                      << " d50=" << dFrost50
                      << " d100=" << dFrost100
                      << " ratio=" << (dFrost50 > 1e-12 ? dFrost100 / dFrost50 : 0.0)
                      << "\n";
            require(dFrost100 > dFrost50 * 1.19,
                    "FROST micro-freeze becomes stronger toward the extreme range", failures);
            require(dShiver50 > 3e-3 && dShiver50 > dShiver25 * 1.25,
                    "SHIVER 50% creates obvious cold time/pitch motion", failures);
            require(dSpace50 > 1e-5, "SPACE 50% is measurably active", failures);

            require(dFrost100 > dFrost50 * 1.19, "FROST 100% stronger than 50%", failures);
            const double shiverExtremeDelta =
                meanAbsDiff(shiver50, shiver100, skip);
            std::cout << "[INFO] SHIVER EXTREME SR=" << static_cast<int>(sr)
                      << " d50=" << dShiver50
                      << " d100=" << dShiver100
                      << " delta50to100=" << shiverExtremeDelta
                      << "\n";
            require(shiverExtremeDelta > 3e-3,
                    "SHIVER 100% is a clearly distinct extreme motion state from 50%", failures);
            require(maxAbs(shiver100) < 8.0,
                    "SHIVER 100% extreme state remains bounded", failures);
            require(dSpace100 > dSpace50 * 1.20, "SPACE 100% stronger than 50%", failures);
            require(meanAbsDiff(frost100, frost100Repeat, 0) < 1e-8,
                    "FROST render is deterministic", failures);

            double silencePeak = 0.0;
            for (float v : frostSilence)
                silencePeak = std::max(silencePeak, std::fabs(static_cast<double>(v)));
            require(silencePeak < 1e-12, "FROST produces no output on silence", failures);

            double textureSilencePeak = 0.0;
            for (float v : textureSilence)
                textureSilencePeak = std::max(textureSilencePeak, std::fabs(static_cast<double>(v)));
            require(textureSilencePeak < 1e-12,
                    "generative material textures produce no output on silence", failures);
            require(meanAbsDiff(textureRepeatA, textureRepeatB, 0) < 1e-8,
                    "generative material textures are deterministic across renders", failures);

            const double dryFund = toneAmplitude(dry, sr, 440.0, skip);
            const double shiverFund = toneAmplitude(shiver50, sr, 440.0, skip);
            const double shiverFundRatio = dryFund > 1e-12 ? shiverFund / dryFund : 0.0;
            require(shiverFundRatio > 0.03,
                    "SHIVER 50% retains a traceable source fundamental while transforming it", failures);

            require(!impulseSpace100.empty() && std::fabs(impulseSpace100.front()) < 1e-6,
                    "SPACE 100% removes the direct impulse and is truly wet", failures);

            const double shiverLowDryAmp = toneAmplitude(shiverLowDry, sr, 80.0, skip);
            const double shiverLowFxAmp = toneAmplitude(shiverLowFx, sr, 80.0, skip);
            const double shiverLowRatio =
                shiverLowDryAmp > 1e-12 ? shiverLowFxAmp / shiverLowDryAmp : 0.0;
            const double coldOnly80Amp = toneAmplitude(coldOnly80, sr, 80.0, skip);
            const double shiverNoCold80Amp = toneAmplitude(shiverOnlyNoCold80, sr, 80.0, skip);
            std::cout << "[INFO] SHIVER 80Hz SR=" << static_cast<int>(sr)
                      << " dry=" << shiverLowDryAmp
                      << " coldOnly=" << coldOnly80Amp
                      << " shiverNoCold=" << shiverNoCold80Amp
                      << " fx=" << shiverLowFxAmp
                      << " ratio=" << shiverLowRatio
                      << " dB=" << (shiverLowRatio > 1e-12
                           ? 20.0 * std::log10(shiverLowRatio) : -200.0)
                      << "\n";
            require(shiverLowRatio < 1.10,
                    "SHIVER does not inflate the protected 80 Hz low band", failures);

            require(dCold100 > dCold50 * 1.12,
                    "COLD 100% remains clearly beyond the 50% signature region", failures);
            const double strongestSingle50 = std::max(
                {dIce50, dMetal50, dFrost50, dShiver50, dSpace50});
            require(dCold100 > strongestSingle50 * 1.10,
                    "COLD 100% exceeds every individual 50% material transformation", failures);

            const auto cold75 = renderSine(sr, 0.8, 440.0, {0.75f, 1.f, 1.f, 1.f, 1.f, 1.f});
            const double cold75Fund = toneAmplitude(cold75, sr, 440.0, skip);
            const double cold75Ratio = dryFund > 1e-12 ? cold75Fund / dryFund : 0.0;
            require(cold75Ratio > 0.005,
                    "COLD 75% retains at least a minimal trace of source pitch identity", failures);

            const size_t early2ms = static_cast<size_t>(sr * 0.002);
            const size_t mid20ms = static_cast<size_t>(sr * 0.020);
            const size_t late100ms = static_cast<size_t>(sr * 0.100);

            auto windowEnergy = [](const std::vector<float>& x, size_t a, size_t b) {
                b = std::min(b, x.size());
                if (b <= a) return 0.0;
                double e = 0.0;
                for (size_t i = a; i < b; ++i)
                    e += static_cast<double>(x[i]) * static_cast<double>(x[i]);
                return e;
            };

            const double iceEarly = windowEnergy(impulseIce100, early2ms, mid20ms);
            const double metalEarly = windowEnergy(impulseMetal100, early2ms, mid20ms);
            const double frostEarly = windowEnergy(impulseFrost100, early2ms, mid20ms);
            const double shiverEarly = windowEnergy(impulseShiver100, early2ms, mid20ms);
            const double spaceLateFingerprint = windowEnergy(impulseSpace100, mid20ms, late100ms);

            std::cout << "[INFO] SPIKE SR=" << static_cast<int>(sr)
                      << " ICE(2-20ms)=" << iceEarly
                      << " METAL(2-20ms)=" << metalEarly
                      << " FROST(2-20ms)=" << frostEarly
                      << " SHIVER(2-20ms)=" << shiverEarly
                      << " SPACE(20-100ms)=" << spaceLateFingerprint << "\n";

            require(iceEarly > 1e-7,
                    "ICE spike produces a distinct crystal/glass time-domain fingerprint", failures);
            require(metalEarly > 1e-7,
                    "METAL spike produces a distinct industrial resonance fingerprint", failures);
            require(frostEarly > 1e-8,
                    "FROST spike produces a distinct frozen-surface fingerprint", failures);
            require(shiverEarly > 1e-8,
                    "SHIVER spike produces a distinct moving-delay fingerprint", failures);
            require(spaceLateFingerprint > 1e-7,
                    "SPACE spike produces a distinct cold-room reflection fingerprint", failures);

            const size_t bloomLateStart = static_cast<size_t>(sr * 0.18);
            const size_t bloomLateEnd = std::min(
                impulseFrozenBloom.size(),
                static_cast<size_t>(sr * 1.20));
            const double bloomLate =
                windowEnergy(impulseFrozenBloom, bloomLateStart, bloomLateEnd);
            std::cout << "[INFO] BLOOM LATE SR=" << static_cast<int>(sr)
                      << " E180ms-1.2s=" << bloomLate << "\n";
            require(bloomLate > 1e-7,
                    "Frozen Bloom creates measurable delayed source-derived bloom energy", failures);

            const size_t tailStart = static_cast<size_t>(sr * 0.002);
            const double dryTail = tailEnergy(impulseDry, tailStart);
            const double ice50Tail = tailEnergy(impulseIce50, tailStart) - dryTail;
            const double ice100Tail = tailEnergy(impulseIce100, tailStart) - dryTail;
            const double metal50Tail = tailEnergy(impulseMetal50, tailStart) - dryTail;
            const double metal100Tail = tailEnergy(impulseMetal100, tailStart) - dryTail;
            const double space50Tail = tailEnergy(impulseSpace50, tailStart) - dryTail;
            const double space100Tail = tailEnergy(impulseSpace100, tailStart) - dryTail;

            require(space50Tail > 1e-6, "SPACE 50% creates sparse early-reflection energy", failures);
            require(space100Tail > space50Tail * 1.20,
                    "SPACE 100% stronger than 50% on impulse", failures);

            const size_t reverbStart = static_cast<size_t>(sr * 0.080);
            const size_t farStart = static_cast<size_t>(sr * 1.000);
            const size_t veryLate = static_cast<size_t>(sr * 3.000);
            const double spaceTailAfter80 = tailEnergy(impulseSpace100, reverbStart);
            const double spaceTailAfter1s = tailEnergy(impulseSpace100, farStart);
            const double spaceTailAfter3s = tailEnergy(impulseSpace100, veryLate);

            std::cout << "[INFO] SPACE LONG SR=" << static_cast<int>(sr)
                      << " E80ms=" << spaceTailAfter80
                      << " E1s=" << spaceTailAfter1s
                      << " E3s=" << spaceTailAfter3s << "\n";

            require(spaceTailAfter80 > 1e-6,
                    "SPACE has a clearly measurable icy reverb tail", failures);
            require(spaceTailAfter1s > 1e-7,
                    "SPACE has a clearly measurable cinematic far-field tail after 1 second", failures);

            const auto stereoSpace = renderStereoImpulse(sr, 3.0, {1.f, 0.f, 0.f, 0.f, 0.f, 1.f});
            const size_t stereoTailStart = static_cast<size_t>(sr * 0.080);
            const double tailCorr = correlation(stereoSpace.left, stereoSpace.right, stereoTailStart);
            require(tailCorr < 0.995,
                    "SPACE tail develops measurable stereo decorrelation", failures);

            const size_t spectralEnd = static_cast<size_t>(sr * 0.500);
            const double lowMidTail =
                0.5 * (spectralMagnitude(stereoSpace.left, sr, 250.0, stereoTailStart, spectralEnd) +
                       spectralMagnitude(stereoSpace.left, sr, 400.0, stereoTailStart, spectralEnd));
            const double upperTail =
                0.5 * (spectralMagnitude(stereoSpace.left, sr, 3000.0, stereoTailStart, spectralEnd) +
                       spectralMagnitude(stereoSpace.left, sr, 5000.0, stereoTailStart, spectralEnd));
            require(upperTail > lowMidTail * 0.80,
                    "SPACE tail avoids warm low-mid bloom and retains upper-band coldness", failures);

            const double coldTailAfter80 = tailEnergy(impulseCold100, reverbStart);
            require(coldTailAfter80 > 1e-6,
                    "COLD 100% engages the cold Space tail as part of the macro", failures);

            const size_t cinematicLate = static_cast<size_t>(sr * 0.180);
            const double cinematicLowTail = tailEnergy(impulseCinematicLow, cinematicLate);
            const double cinematicHighTail = tailEnergy(impulseCinematicHigh, cinematicLate);
            require(cinematicHighTail > 1e-6,
                    "high COLD creates a measurable cinematic distant cloud without SPACE", failures);
            require(cinematicHighTail > cinematicLowTail * 4.0,
                    "cinematic distant layer escalates strongly above the subtle COLD range", failures);

            require(ice100Tail > ice50Tail * 1.25,
                    "ICE 100% stronger than 50% on transient excitation", failures);
            require(metal100Tail > metal50Tail * 1.25,
                    "METAL 100% stronger than 50% on transient excitation", failures);

            {
                const auto chordDry = renderChord(sr, 0.8, {});
                const auto chordCold = renderChord(sr, 0.8, {0.75f, 0.65f, 0.80f, 0.25f, 0.20f, 0.30f});
                for (double chordHz : {220.0, 277.183, 329.628})
                {
                    const double ref = toneAmplitude(chordDry, sr, chordHz, skip);
                    const double fx = toneAmplitude(chordCold, sr, chordHz, skip);
                    const double ratio = ref > 1e-12 ? fx / ref : 0.0;
                    require(ratio > 0.02,
                            "strong cold processing retains a trace of each chord tone at " +
                            std::to_string(static_cast<int>(chordHz)) + " Hz",
                            failures);
                }
            }

            for (double noteHz : {220.0, 440.0, 659.255, 880.0})
            {
                const auto reference = renderSine(sr, 0.8, noteHz, {});
                const auto effected = renderSine(sr, 0.8, noteHz, {0.65f, 0.65f, 0.80f});
                const double refAmp = toneAmplitude(reference, sr, noteHz, skip);
                const double fxAmp = toneAmplitude(effected, sr, noteHz, skip);
                const double ratio = refAmp > 1e-12 ? fxAmp / refAmp : 0.0;

                require(ratio > 0.02,
                        "source pitch remains traceable under extreme transformation at " + std::to_string(static_cast<int>(noteHz)) +
                        " Hz / SR " + std::to_string(static_cast<int>(sr)),
                        failures);
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[EXCEPTION] " << e.what() << "\n";
        return 2;
    }

    if (failures != 0)
        std::cerr << failures << " test(s) failed\n";
    else
        std::cout << "All Colderator core measurements passed\n";

    return failures == 0 ? 0 : 1;
}
