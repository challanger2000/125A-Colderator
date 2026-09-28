#include "processor.h"
#include "parameters.h"

#include "pluginterfaces/vst/ivstaudioprocessor.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Steinberg;
using namespace Steinberg::Vst;
using Colderator::Processor;

namespace {

struct Wav
{
    int sampleRate = 0;
    int channels = 0;
    std::vector<float> interleaved;
};

std::uint16_t readU16(std::ifstream& f)
{
    unsigned char b[2] {};
    f.read(reinterpret_cast<char*>(b), 2);
    return static_cast<std::uint16_t>(b[0] | (b[1] << 8));
}

std::uint32_t readU32(std::ifstream& f)
{
    unsigned char b[4] {};
    f.read(reinterpret_cast<char*>(b), 4);
    return static_cast<std::uint32_t>(
        b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
}

Wav loadPcm16Wav(const std::filesystem::path& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        throw std::runtime_error("cannot open " + path.string());

    char riff[4] {}, wave[4] {};
    f.read(riff, 4);
    (void)readU32(f);
    f.read(wave, 4);
    if (std::string(riff, 4) != "RIFF" || std::string(wave, 4) != "WAVE")
        throw std::runtime_error("not RIFF/WAVE: " + path.string());

    std::uint16_t format = 0, channels = 0, bits = 0;
    std::uint32_t sampleRate = 0;
    std::vector<std::int16_t> pcm;

    while (f && (!format || pcm.empty()))
    {
        char id[4] {};
        f.read(id, 4);
        if (!f) break;
        const std::uint32_t size = readU32(f);
        const std::string chunk(id, 4);

        if (chunk == "fmt ")
        {
            format = readU16(f);
            channels = readU16(f);
            sampleRate = readU32(f);
            (void)readU32(f);
            (void)readU16(f);
            bits = readU16(f);
            if (size > 16)
                f.seekg(static_cast<std::streamoff>(size - 16), std::ios::cur);
        }
        else if (chunk == "data")
        {
            if (size % 2 != 0)
                throw std::runtime_error("odd PCM data size");
            pcm.resize(size / 2);
            f.read(reinterpret_cast<char*>(pcm.data()), static_cast<std::streamsize>(size));
        }
        else
        {
            f.seekg(static_cast<std::streamoff>(size), std::ios::cur);
        }

        if (size & 1u)
            f.seekg(1, std::ios::cur);
    }

    if (format != 1 || bits != 16 || (channels != 1 && channels != 2) || sampleRate == 0 || pcm.empty())
        throw std::runtime_error("unsupported fixture WAV: " + path.string());

    Wav w;
    w.sampleRate = static_cast<int>(sampleRate);
    w.channels = static_cast<int>(channels);
    w.interleaved.resize(pcm.size());
    for (std::size_t i = 0; i < pcm.size(); ++i)
        w.interleaved[i] = static_cast<float>(pcm[i]) / 32768.f;
    return w;
}

std::vector<float> renderLeft(const Wav& wav, bool processed)
{
    constexpr int block = 128;
    Processor p;
    if (p.initialize(nullptr) != kResultOk)
        throw std::runtime_error("processor initialize failed");

    ProcessSetup setup {};
    setup.processMode = kOffline;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = block;
    setup.sampleRate = wav.sampleRate;
    if (p.setupProcessing(setup) != kResultOk)
        throw std::runtime_error("setup failed");

    if (processed)
    {
        p.setTestParameter(Colderator::kCold, 0.72f);
        p.setTestParameter(Colderator::kIce, 0.42f);
        p.setTestParameter(Colderator::kMetal, 0.58f);
        p.setTestParameter(Colderator::kFrost, 0.38f);
        p.setTestParameter(Colderator::kShiver, 0.24f);
        p.setTestParameter(Colderator::kSpace, 0.44f);
        p.setTestParameter(Colderator::kAtmosAType, 7.f / 9.f);
        p.setTestParameter(Colderator::kAtmosAAmount, 0.28f);
        p.setTestParameter(Colderator::kAtmosBType, 4.f / 9.f);
        p.setTestParameter(Colderator::kAtmosBAmount, 0.18f);
    }
    p.setTestParameter(Colderator::kOutput, 0.5f);

    if (p.setActive(true) != kResultOk)
        throw std::runtime_error("activate failed");

    const std::size_t frames = wav.interleaved.size() / static_cast<std::size_t>(wav.channels);
    std::vector<float> result(frames, 0.f);
    std::vector<float> inL(block, 0.f), inR(block, 0.f), outL(block, 0.f), outR(block, 0.f);
    float* inPtrs[2] = {inL.data(), inR.data()};
    float* outPtrs[2] = {outL.data(), outR.data()};
    AudioBusBuffers inBus {};
    inBus.numChannels = 2;
    inBus.channelBuffers32 = inPtrs;
    AudioBusBuffers outBus {};
    outBus.numChannels = 2;
    outBus.channelBuffers32 = outPtrs;

    std::size_t pos = 0;
    while (pos < frames)
    {
        const int n = static_cast<int>(std::min<std::size_t>(block, frames - pos));
        for (int i = 0; i < n; ++i)
        {
            const std::size_t frame = pos + static_cast<std::size_t>(i);
            inL[i] = wav.interleaved[frame * wav.channels];
            inR[i] = wav.channels == 2
                ? wav.interleaved[frame * wav.channels + 1]
                : inL[i];
            outL[i] = outR[i] = 0.f;
        }

        ProcessData data {};
        data.processMode = kOffline;
        data.symbolicSampleSize = kSample32;
        data.numSamples = n;
        data.numInputs = 1;
        data.numOutputs = 1;
        data.inputs = &inBus;
        data.outputs = &outBus;
        if (p.process(data) != kResultOk)
            throw std::runtime_error("process failed");

        for (int i = 0; i < n; ++i)
            result[pos + static_cast<std::size_t>(i)] = outL[i];
        pos += static_cast<std::size_t>(n);
    }

    p.setActive(false);
    p.terminate();
    return result;
}

double meanAbsDiff(const std::vector<float>& a, const std::vector<float>& b)
{
    const std::size_t n = std::min(a.size(), b.size());
    if (n == 0) return 0.0;
    double sum = 0.0;
    for (std::size_t i = 0; i < n; ++i)
        sum += std::fabs(static_cast<double>(a[i]) - static_cast<double>(b[i]));
    return sum / static_cast<double>(n);
}

double maxAbs(const std::vector<float>& x)
{
    double m = 0.0;
    for (float v : x)
        m = std::max(m, std::fabs(static_cast<double>(v)));
    return m;
}

bool finite(const std::vector<float>& x)
{
    return std::all_of(x.begin(), x.end(), [](float v) { return std::isfinite(v); });
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "usage: ColderatorRealFixtureMeasurements <fixture-dir>\n";
        return 2;
    }

    const std::filesystem::path root = argv[1];
    const std::vector<std::string> names = {"piano.wav", "drums.wav", "synth_pad.wav", "voice.wav"};

    int failures = 0;
    for (const auto& name : names)
    {
        try
        {
            const auto wav = loadPcm16Wav(root / name);
            if (wav.sampleRate != 48000)
            {
                std::cerr << "[FAIL] " << name << " fixture rate is not 48 kHz\n";
                ++failures;
                continue;
            }

            const auto dry = renderLeft(wav, false);
            const auto fx = renderLeft(wav, true);

            std::vector<float> source(dry.size(), 0.f);
            for (std::size_t i = 0; i < source.size(); ++i)
                source[i] = wav.interleaved[i * wav.channels];

            const double dryNull = meanAbsDiff(dry, source);
            const double fxDelta = meanAbsDiff(fx, source);
            const double peak = maxAbs(fx);

            std::cout << "[REAL] " << name
                      << " frames=" << source.size()
                      << " dryNull=" << dryNull
                      << " fxDelta=" << fxDelta
                      << " peak=" << peak << "\n";

            if (dryNull > 2.0e-6)
            {
                std::cerr << "[FAIL] " << name << " COLD=0/output=0dB is not neutral\n";
                ++failures;
            }
            if (!finite(fx) || peak >= 8.0)
            {
                std::cerr << "[FAIL] " << name << " processed output is invalid/unbounded\n";
                ++failures;
            }
            if (fxDelta <= 2.0e-4)
            {
                std::cerr << "[FAIL] " << name << " strong real-program fixture is not measurably transformed\n";
                ++failures;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "[FAIL] " << name << ": " << e.what() << "\n";
            ++failures;
        }
    }

    if (failures == 0)
        std::cout << "All real-audio fixture measurements passed\n";
    return failures == 0 ? 0 : 1;
}
