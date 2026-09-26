#include "processor.h"
#include "parameters.h"

#include "pluginterfaces/vst/ivstaudioprocessor.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Steinberg;
using namespace Steinberg::Vst;
using Colderator::Processor;

namespace {

constexpr double kPi = 3.14159265358979323846;

struct Settings
{
    float cold = 0.f;
    float ice = 0.f;
    float metal = 0.f;
    float frost = 0.f;
    float shiver = 0.f;
    float space = 0.f;
};

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

        for (double sr : {44100.0, 48000.0, 96000.0})
        {
            const size_t skip = static_cast<size_t>(sr * 0.15);

            const auto dry = renderSine(sr, 0.8, 440.0, {});
            const auto cold50 = renderSine(sr, 0.8, 440.0, {0.50f, 0.f, 0.f});
            const auto cold100 = renderSine(sr, 0.8, 440.0, {1.f, 0.f, 0.f});
            const auto ice50 = renderSine(sr, 0.8, 440.0, {0.f, 0.50f, 0.f});
            const auto ice100 = renderSine(sr, 0.8, 440.0, {0.f, 1.f, 0.f});
            const auto metal50 = renderSine(sr, 0.8, 440.0, {0.f, 0.f, 0.50f});
            const auto metal100 = renderSine(sr, 0.8, 440.0, {0.f, 0.f, 1.f});
            const auto frost50 = renderSine(sr, 0.8, 440.0, {0.f, 0.f, 0.f, 0.50f, 0.f});
            const auto frost100 = renderSine(sr, 0.8, 440.0, {0.f, 0.f, 0.f, 1.f, 0.f});
            const auto frost100Repeat = renderSine(sr, 0.8, 440.0, {0.f, 0.f, 0.f, 1.f, 0.f});
            const auto frostSilence = renderSine(sr, 0.8, 0.0, {0.f, 0.f, 0.f, 1.f, 0.f});
            const auto shiver50 = renderSine(sr, 0.8, 440.0, {0.f, 0.f, 0.f, 0.f, 0.50f});
            const auto shiver100 = renderSine(sr, 0.8, 440.0, {0.f, 0.f, 0.f, 0.f, 1.f});
            const auto space50 = renderSine(sr, 0.8, 440.0, {0.f, 0.f, 0.f, 0.f, 0.f, 0.50f});
            const auto space75 = renderSine(sr, 0.8, 440.0, {0.f, 0.f, 0.f, 0.f, 0.f, 0.75f});
            const auto space100 = renderSine(sr, 0.8, 440.0, {0.f, 0.f, 0.f, 0.f, 0.f, 1.f});

            const auto impulseDry = renderImpulse(sr, 0.25, {});
            const auto impulseIce50 = renderImpulse(sr, 0.25, {0.f, 0.50f, 0.f});
            const auto impulseIce100 = renderImpulse(sr, 0.25, {0.f, 1.f, 0.f});
            const auto impulseMetal50 = renderImpulse(sr, 0.25, {0.f, 0.f, 0.50f});
            const auto impulseMetal100 = renderImpulse(sr, 0.25, {0.f, 0.f, 1.f});
            const auto impulseSpace50 = renderImpulse(sr, 1.6, {0.f, 0.f, 0.f, 0.f, 0.f, 0.50f});
            const auto impulseSpace100 = renderImpulse(sr, 1.6, {0.f, 0.f, 0.f, 0.f, 0.f, 1.f});
            const auto impulseCold100 = renderImpulse(sr, 1.6, {1.f, 0.f, 0.f, 0.f, 0.f, 0.f});

            require(finiteBuffer(dry) && finiteBuffer(cold100) &&
                    finiteBuffer(ice100) && finiteBuffer(metal100) &&
                    finiteBuffer(frost100) && finiteBuffer(shiver100) &&
                    finiteBuffer(space100),
                    "finite output at " + std::to_string(static_cast<int>(sr)) + " Hz",
                    failures);

            require(meanAbsDiff(dry, renderSine(sr, 0.8, 440.0, {}), skip) < 1e-8,
                    "neutral render deterministic at " + std::to_string(static_cast<int>(sr)) + " Hz",
                    failures);

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

            const double dCold50 = meanAbsDiff(dry, cold50, skip);
            const double dCold100 = meanAbsDiff(dry, cold100, skip);
            const double dIce50 = meanAbsDiff(dry, ice50, skip);
            const double dIce100 = meanAbsDiff(dry, ice100, skip);
            const double dMetal50 = meanAbsDiff(dry, metal50, skip);
            const double dMetal100 = meanAbsDiff(dry, metal100, skip);
            const double dFrost50 = meanAbsDiff(dry, frost50, skip);
            const double dFrost100 = meanAbsDiff(dry, frost100, skip);
            const double dShiver50 = meanAbsDiff(dry, shiver50, skip);
            const double dShiver100 = meanAbsDiff(dry, shiver100, skip);
            const double dSpace50 = meanAbsDiff(dry, space50, skip);
            const double dSpace100 = meanAbsDiff(dry, space100, skip);

            require(dCold50 > 1e-4, "COLD 50% is measurably active", failures);
            require(dIce50 > 1e-5, "ICE 50% is measurably active", failures);
            require(dMetal50 > 1e-5, "METAL 50% is measurably active", failures);
            require(dFrost50 > 1e-5, "FROST 50% is measurably active", failures);
            require(dShiver50 > 1e-5, "SHIVER 50% is measurably active", failures);
            require(dSpace50 > 1e-5, "SPACE 50% is measurably active", failures);

            require(dFrost100 > dFrost50 * 1.20, "FROST 100% stronger than 50%", failures);
            require(dShiver100 > dShiver50 * 1.20, "SHIVER 100% stronger than 50%", failures);
            require(dSpace100 > dSpace50 * 1.20, "SPACE 100% stronger than 50%", failures);
            require(meanAbsDiff(frost100, frost100Repeat, 0) < 1e-8,
                    "FROST render is deterministic", failures);

            double silencePeak = 0.0;
            for (float v : frostSilence)
                silencePeak = std::max(silencePeak, std::fabs(static_cast<double>(v)));
            require(silencePeak < 1e-12, "FROST produces no output on silence", failures);

            const double dryFund = toneAmplitude(dry, sr, 440.0, skip);
            const double shiverFund = toneAmplitude(shiver100, sr, 440.0, skip);
            const double shiverFundRatio = dryFund > 1e-12 ? shiverFund / dryFund : 0.0;
            require(shiverFundRatio > 0.90 && shiverFundRatio < 1.10,
                    "SHIVER preserves sustained fundamental amplitude", failures);

            const double spaceFund = toneAmplitude(space75, sr, 440.0, skip);
            const double spaceFundRatio = dryFund > 1e-12 ? spaceFund / dryFund : 0.0;
            require(spaceFundRatio > 0.75 && spaceFundRatio < 1.25,
                    "SPACE 75% preserves sustained fundamental amplitude", failures);

            require(dCold100 > dCold50 * 1.35, "COLD 100% clearly stronger than 50%", failures);
            require(dCold100 > dIce50 + dMetal50,
                    "COLD 100% behaves as a compound character macro", failures);

            const auto cold75 = renderSine(sr, 0.8, 440.0, {0.75f, 0.f, 0.f, 0.f, 0.f, 0.f});
            const double cold75Fund = toneAmplitude(cold75, sr, 440.0, skip);
            const double cold75Ratio = dryFund > 1e-12 ? cold75Fund / dryFund : 0.0;
            require(cold75Ratio > 0.35,
                    "COLD 75% retains clear fundamental identity", failures);

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
            const size_t reverbLate = static_cast<size_t>(sr * 0.900);
            const double spaceTailAfter80 = tailEnergy(impulseSpace100, reverbStart);
            const double spaceTailAfter900 = tailEnergy(impulseSpace100, reverbLate);

            require(spaceTailAfter80 > 1e-6,
                    "SPACE has a clearly measurable icy reverb tail", failures);
            require(spaceTailAfter900 < spaceTailAfter80 * 0.25,
                    "SPACE reverb decays substantially by 900 ms", failures);

            const double coldTailAfter80 = tailEnergy(impulseCold100, reverbStart);
            require(coldTailAfter80 > 1e-6,
                    "COLD 100% engages the cold Space tail as part of the macro", failures);

            require(ice100Tail > ice50Tail * 1.25,
                    "ICE 100% stronger than 50% on transient excitation", failures);
            require(metal100Tail > metal50Tail * 1.25,
                    "METAL 100% stronger than 50% on transient excitation", failures);

            for (double noteHz : {220.0, 440.0, 659.255, 880.0})
            {
                const auto reference = renderSine(sr, 0.8, noteHz, {});
                const auto effected = renderSine(sr, 0.8, noteHz, {0.65f, 0.65f, 0.80f});
                const double refAmp = toneAmplitude(reference, sr, noteHz, skip);
                const double fxAmp = toneAmplitude(effected, sr, noteHz, skip);
                const double ratio = refAmp > 1e-12 ? fxAmp / refAmp : 0.0;

                require(ratio > 0.25,
                        "fundamental retained at " + std::to_string(static_cast<int>(noteHz)) +
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
