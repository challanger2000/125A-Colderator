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
};

std::vector<float> renderSine(double sr, double seconds, double hz, const Settings& settings)
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

    p.setTestParameter(Colderator::kCold, settings.cold);
    p.setTestParameter(Colderator::kIce, settings.ice);
    p.setTestParameter(Colderator::kMetal, settings.metal);
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

            require(finiteBuffer(dry) && finiteBuffer(cold100) &&
                    finiteBuffer(ice100) && finiteBuffer(metal100),
                    "finite output at " + std::to_string(static_cast<int>(sr)) + " Hz",
                    failures);

            require(meanAbsDiff(dry, renderSine(sr, 0.8, 440.0, {}), skip) < 1e-8,
                    "neutral render deterministic at " + std::to_string(static_cast<int>(sr)) + " Hz",
                    failures);

            const double dCold50 = meanAbsDiff(dry, cold50, skip);
            const double dCold100 = meanAbsDiff(dry, cold100, skip);
            const double dIce50 = meanAbsDiff(dry, ice50, skip);
            const double dIce100 = meanAbsDiff(dry, ice100, skip);
            const double dMetal50 = meanAbsDiff(dry, metal50, skip);
            const double dMetal100 = meanAbsDiff(dry, metal100, skip);

            require(dCold50 > 1e-4, "COLD 50% is measurably active", failures);
            require(dIce50 > 1e-5, "ICE 50% is measurably active", failures);
            require(dMetal50 > 1e-5, "METAL 50% is measurably active", failures);

            require(dCold100 > dCold50 * 1.25, "COLD 100% stronger than 50%", failures);
            require(dIce100 > dIce50 * 1.25, "ICE 100% stronger than 50%", failures);
            require(dMetal100 > dMetal50 * 1.25, "METAL 100% stronger than 50%", failures);

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
