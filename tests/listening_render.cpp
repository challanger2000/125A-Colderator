#include "processor.h"
#include "parameters.h"

#include "pluginterfaces/vst/ivstaudioprocessor.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Steinberg;
using namespace Steinberg::Vst;
using Colderator::Processor;

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kSr = 48000.0;
constexpr int kBlock = 128;
constexpr double kSourceSeconds = 6.0;
constexpr double kTotalSeconds = 10.0;

struct Stereo
{
    std::vector<float> l;
    std::vector<float> r;
};

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
    float atmosAType = 0.f;
    float atmosAAmount = 0.f;
    float atmosBType = 0.f;
    float atmosBAmount = 0.f;
};

inline float hashNoise(std::uint32_t x)
{
    x ^= x >> 17;
    x *= 0xed5ad4bbu;
    x ^= x >> 11;
    x *= 0xac4c1b51u;
    x ^= x >> 15;
    x *= 0x31848babu;
    x ^= x >> 14;
    return (static_cast<float>(x & 0x00ffffffu) / 8388607.5f) - 1.f;
}

inline float softClip(float x)
{
    return std::tanh(x);
}

std::vector<float> makeWarmPad()
{
    const std::size_t n = static_cast<std::size_t>(kSr * kTotalSeconds);
    const std::size_t active = static_cast<std::size_t>(kSr * kSourceSeconds);
    std::vector<float> x(n, 0.f);
    constexpr double f[] = {110.0, 164.8138, 220.0, 261.6256, 329.6276};
    for (std::size_t i = 0; i < active; ++i)
    {
        const double t = static_cast<double>(i) / kSr;
        const double fadeIn = std::min(1.0, t / 0.65);
        const double fadeOut = std::min(1.0, (kSourceSeconds - t) / 0.85);
        const double env = std::max(0.0, std::min(fadeIn, fadeOut));
        const double drift = 1.0 + 0.0014 * std::sin(2.0 * kPi * 0.17 * t);
        double s = 0.0;
        for (int v = 0; v < 5; ++v)
        {
            const double ph = 2.0 * kPi * f[v] * drift * t;
            s += 0.68 * std::sin(ph);
            s += 0.22 * std::sin(2.0 * ph);
            s += 0.08 * std::sin(3.0 * ph);
        }
        const double breath = 0.88 + 0.12 * std::sin(2.0 * kPi * 0.11 * t);
        x[i] = static_cast<float>(0.045 * env * breath * s);
    }
    return x;
}

std::vector<float> makeKeys()
{
    const std::size_t n = static_cast<std::size_t>(kSr * kTotalSeconds);
    const std::size_t active = static_cast<std::size_t>(kSr * kSourceSeconds);
    std::vector<float> x(n, 0.f);
    constexpr double chords[4][3] = {
        {130.8128, 155.5635, 196.0},
        {146.8324, 174.6141, 220.0},
        {110.0, 130.8128, 164.8138},
        {123.4708, 146.8324, 184.9972}
    };
    for (std::size_t i = 0; i < active; ++i)
    {
        const double t = static_cast<double>(i) / kSr;
        const int hit = static_cast<int>(t / 1.5);
        const double local = t - 1.5 * hit;
        if (local > 1.30)
            continue;
        const double env = (1.0 - std::exp(-local * 42.0)) * std::exp(-local * 2.15);
        double s = 0.0;
        for (double f : chords[hit % 4])
        {
            s += std::sin(2.0 * kPi * f * local);
            s += 0.34 * std::sin(2.0 * kPi * f * 2.01 * local);
            s += 0.12 * std::sin(2.0 * kPi * f * 3.98 * local);
        }
        x[i] = static_cast<float>(0.075 * env * s);
    }
    return x;
}

std::vector<float> makeDrums()
{
    const std::size_t n = static_cast<std::size_t>(kSr * kTotalSeconds);
    const std::size_t active = static_cast<std::size_t>(kSr * kSourceSeconds);
    std::vector<float> x(n, 0.f);
    for (std::size_t i = 0; i < active; ++i)
    {
        const double t = static_cast<double>(i) / kSr;
        float s = 0.f;

        const double beat = std::fmod(t, 0.5);
        if (beat < 0.22)
        {
            const double e = std::exp(-beat * 18.0);
            const double freq = 48.0 + 58.0 * std::exp(-beat * 30.0);
            s += static_cast<float>(0.48 * e * std::sin(2.0 * kPi * freq * beat));
        }

        const double sn = std::fmod(t + 0.5, 1.0);
        if (sn < 0.18)
        {
            const double e = std::exp(-sn * 22.0);
            const float n0 = hashNoise(static_cast<std::uint32_t>(i * 17u + 991u));
            s += static_cast<float>(0.20 * e) * n0;
            s += static_cast<float>(0.08 * e * std::sin(2.0 * kPi * 185.0 * sn));
        }

        const double hh = std::fmod(t, 0.25);
        if (hh < 0.055)
        {
            const double e = std::exp(-hh * 65.0);
            const float n0 = hashNoise(static_cast<std::uint32_t>(i * 31u + 1237u));
            const float n1 = hashNoise(static_cast<std::uint32_t>((i > 0 ? i - 1 : 0) * 31u + 1237u));
            s += static_cast<float>(0.08 * e) * (n0 - n1);
        }

        x[i] = softClip(s);
    }
    return x;
}

std::vector<float> makeMiniMix()
{
    auto pad = makeWarmPad();
    auto keys = makeKeys();
    auto drums = makeDrums();
    std::vector<float> x(pad.size(), 0.f);
    for (std::size_t i = 0; i < x.size(); ++i)
    {
        const double t = static_cast<double>(i) / kSr;
        const float bass = i < static_cast<std::size_t>(kSr * kSourceSeconds)
            ? static_cast<float>(0.09 * std::sin(2.0 * kPi * 55.0 * t))
            : 0.f;
        x[i] = softClip(0.75f * pad[i] + 0.65f * keys[i] + 0.75f * drums[i] + bass);
    }
    return x;
}

void setParams(Processor& p, const Settings& s)
{
    p.setTestParameter(Colderator::kCold, s.cold);
    p.setTestParameter(Colderator::kIce, s.ice);
    p.setTestParameter(Colderator::kMetal, s.metal);
    p.setTestParameter(Colderator::kFrost, s.frost);
    p.setTestParameter(Colderator::kShiver, s.shiver);
    p.setTestParameter(Colderator::kSpace, s.space);
    p.setTestParameter(Colderator::kIceMaterial, s.iceMaterial);
    p.setTestParameter(Colderator::kMetalMaterial, s.metalMaterial);
    p.setTestParameter(Colderator::kFrostMaterial, s.frostMaterial);
    p.setTestParameter(Colderator::kShiverMaterial, s.shiverMaterial);
    p.setTestParameter(Colderator::kSpaceMaterial, s.spaceMaterial);
    p.setTestParameter(Colderator::kAtmosAType, s.atmosAType);
    p.setTestParameter(Colderator::kAtmosAAmount, s.atmosAAmount);
    p.setTestParameter(Colderator::kAtmosBType, s.atmosBType);
    p.setTestParameter(Colderator::kAtmosBAmount, s.atmosBAmount);
    p.setTestParameter(Colderator::kOutput, 0.5f);
}

Stereo render(const std::vector<float>& input, const Settings& settings)
{
    Processor p;
    if (p.initialize(nullptr) != kResultOk)
        throw std::runtime_error("initialize failed");

    ProcessSetup setup {};
    setup.processMode = kOffline;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = kBlock;
    setup.sampleRate = kSr;
    if (p.setupProcessing(setup) != kResultOk)
        throw std::runtime_error("setupProcessing failed");

    setParams(p, settings);
    if (p.setActive(true) != kResultOk)
        throw std::runtime_error("setActive failed");

    Stereo result;
    result.l.resize(input.size());
    result.r.resize(input.size());

    std::vector<float> inL(kBlock), inR(kBlock), outL(kBlock), outR(kBlock);
    float* inPtrs[2] = {inL.data(), inR.data()};
    float* outPtrs[2] = {outL.data(), outR.data()};

    AudioBusBuffers inBus {};
    inBus.numChannels = 2;
    inBus.channelBuffers32 = inPtrs;
    AudioBusBuffers outBus {};
    outBus.numChannels = 2;
    outBus.channelBuffers32 = outPtrs;

    std::size_t pos = 0;
    while (pos < input.size())
    {
        const int count = static_cast<int>(std::min<std::size_t>(kBlock, input.size() - pos));
        for (int i = 0; i < count; ++i)
        {
            inL[i] = input[pos + static_cast<std::size_t>(i)];
            inR[i] = input[pos + static_cast<std::size_t>(i)];
            outL[i] = outR[i] = 0.f;
        }

        ProcessData data {};
        data.processMode = kOffline;
        data.symbolicSampleSize = kSample32;
        data.numSamples = count;
        data.numInputs = 1;
        data.numOutputs = 1;
        data.inputs = &inBus;
        data.outputs = &outBus;
        if (p.process(data) != kResultOk)
            throw std::runtime_error("process failed");

        for (int i = 0; i < count; ++i)
        {
            result.l[pos + static_cast<std::size_t>(i)] = outL[i];
            result.r[pos + static_cast<std::size_t>(i)] = outR[i];
        }
        pos += static_cast<std::size_t>(count);
    }

    p.setActive(false);
    p.terminate();
    return result;
}

void writeU16(std::ofstream& f, std::uint16_t v)
{
    char b[2] = {static_cast<char>(v & 0xff), static_cast<char>((v >> 8) & 0xff)};
    f.write(b, 2);
}
void writeU32(std::ofstream& f, std::uint32_t v)
{
    char b[4] = {
        static_cast<char>(v & 0xff), static_cast<char>((v >> 8) & 0xff),
        static_cast<char>((v >> 16) & 0xff), static_cast<char>((v >> 24) & 0xff)
    };
    f.write(b, 4);
}

void writeWav(const std::filesystem::path& path, const Stereo& x)
{
    const std::size_t frames = std::min(x.l.size(), x.r.size());
    const std::uint32_t dataBytes = static_cast<std::uint32_t>(frames * 2u * 2u);
    std::ofstream f(path, std::ios::binary);
    if (!f)
        throw std::runtime_error("cannot open WAV output");

    f.write("RIFF", 4);
    writeU32(f, 36u + dataBytes);
    f.write("WAVE", 4);
    f.write("fmt ", 4);
    writeU32(f, 16u);
    writeU16(f, 1u);
    writeU16(f, 2u);
    writeU32(f, static_cast<std::uint32_t>(kSr));
    writeU32(f, static_cast<std::uint32_t>(kSr) * 4u);
    writeU16(f, 4u);
    writeU16(f, 16u);
    f.write("data", 4);
    writeU32(f, dataBytes);

    for (std::size_t i = 0; i < frames; ++i)
    {
        for (float v : {x.l[i], x.r[i]})
        {
            v = std::max(-1.f, std::min(1.f, v));
            const auto q = static_cast<std::int16_t>(std::lrint(v * 32767.f));
            writeU16(f, static_cast<std::uint16_t>(q));
        }
    }
}

Settings preset(int stage)
{
    Settings s {};
    if (stage == 0)
        return s;

    if (stage == 25)
    {
        s.cold = 0.25f; s.ice = 0.70f; s.metal = 0.38f; s.frost = 0.65f;
        s.shiver = 0.28f; s.space = 0.38f;
        s.iceMaterial = 0.f; s.metalMaterial = 0.f; s.frostMaterial = 0.f;
        s.shiverMaterial = 0.f; s.spaceMaterial = 0.f;
        s.atmosAType = 1.f / 9.f; s.atmosAAmount = 0.10f; // Storm
    }
    else if (stage == 50)
    {
        s.cold = 0.50f; s.ice = 0.80f; s.metal = 0.48f; s.frost = 0.78f;
        s.shiver = 0.38f; s.space = 0.58f;
        s.iceMaterial = 1.f / 5.f; s.metalMaterial = 4.f / 5.f;
        s.frostMaterial = 4.f / 5.f; s.shiverMaterial = 1.f / 5.f;
        s.spaceMaterial = 4.f / 5.f;
        s.atmosAType = 1.f / 9.f; s.atmosAAmount = 0.30f; // Storm
        s.atmosBType = 2.f / 9.f; s.atmosBAmount = 0.18f; // Drone
    }
    else if (stage == 75)
    {
        s.cold = 0.75f; s.ice = 0.88f; s.metal = 0.62f; s.frost = 0.88f;
        s.shiver = 0.46f; s.space = 0.76f;
        s.iceMaterial = 4.f / 5.f; s.metalMaterial = 4.f / 5.f;
        s.frostMaterial = 5.f / 5.f; s.shiverMaterial = 3.f / 5.f;
        s.spaceMaterial = 5.f / 5.f;
        s.atmosAType = 1.f / 9.f; s.atmosAAmount = 0.50f;
        s.atmosBType = 2.f / 9.f; s.atmosBAmount = 0.32f;
    }
    else
    {
        s.cold = 1.00f; s.ice = 0.95f; s.metal = 0.78f; s.frost = 1.00f;
        s.shiver = 0.62f; s.space = 0.92f;
        s.iceMaterial = 5.f / 5.f; s.metalMaterial = 4.f / 5.f;
        s.frostMaterial = 5.f / 5.f; s.shiverMaterial = 4.f / 5.f;
        s.spaceMaterial = 5.f / 5.f;
        s.atmosAType = 1.f / 9.f; s.atmosAAmount = 0.72f;
        s.atmosBType = 2.f / 9.f; s.atmosBAmount = 0.52f;
    }
    return s;
}

} // namespace

int main(int argc, char** argv)
{
    try
    {
        const std::filesystem::path out = argc > 1 ? argv[1] : "listening-pack";
        std::filesystem::create_directories(out);

        struct Source { const char* name; std::vector<float> audio; };
        std::vector<Source> sources;
        sources.push_back({"WarmPad", makeWarmPad()});
        sources.push_back({"Keys", makeKeys()});
        sources.push_back({"Drums", makeDrums()});
        sources.push_back({"MiniMix", makeMiniMix()});

        constexpr int stages[] = {0, 25, 50, 75, 100};
        for (const auto& src : sources)
        {
            for (int stage : stages)
            {
                const auto rendered = render(src.audio, preset(stage));
                const std::string suffix = stage == 0 ? "Dry" : ("Cold" + std::to_string(stage));
                const auto path = out / (std::string(src.name) + "_" + suffix + ".wav");
                writeWav(path, rendered);
                std::cout << "WROTE " << path.string() << "\n";
            }
        }

        auto bloomSettings = [](float amount) {
            Settings b {};
            b.cold = 1.f;
            b.ice = 0.45f;
            b.frost = 0.35f;
            b.space = 0.28f;
            b.atmosAType = 8.f / 9.f; // Frozen Bloom
            b.atmosAAmount = amount;
            return b;
        };

        const auto warmPad = makeWarmPad();
        const auto keys = makeKeys();
        writeWav(out / "WarmPad_FrozenBloom50.wav", render(warmPad, bloomSettings(0.50f)));
        writeWav(out / "WarmPad_FrozenBloom80.wav", render(warmPad, bloomSettings(0.80f)));
        writeWav(out / "Keys_FrozenBloom50.wav", render(keys, bloomSettings(0.50f)));
        writeWav(out / "Keys_FrozenBloom80.wav", render(keys, bloomSettings(0.80f)));

        auto machineSettings = [](float amount) {
            Settings m {};
            m.cold = 1.f;
            m.metal = 0.28f;
            m.space = 0.22f;
            m.atmosAType = 9.f / 9.f; // Machine
            m.atmosAAmount = amount;
            return m;
        };

        const auto drums = makeDrums();
        const auto miniMix = makeMiniMix();
        writeWav(out / "Drums_Machine50.wav", render(drums, machineSettings(0.50f)));
        writeWav(out / "Drums_Machine80.wav", render(drums, machineSettings(0.80f)));
        writeWav(out / "MiniMix_Machine50.wav", render(miniMix, machineSettings(0.50f)));
        writeWav(out / "MiniMix_Machine80.wav", render(miniMix, machineSettings(0.80f)));

        auto distantMetalSettings = [](float amount) {
            Settings m {};
            m.cold = 1.f;
            m.space = 0.24f;
            m.atmosAType = 4.f / 9.f; // Distant Metal
            m.atmosAAmount = amount;
            return m;
        };

        writeWav(out / "Keys_DistantMetal50.wav", render(keys, distantMetalSettings(0.50f)));
        writeWav(out / "Keys_DistantMetal80.wav", render(keys, distantMetalSettings(0.80f)));
        writeWav(out / "MiniMix_DistantMetal50.wav", render(miniMix, distantMetalSettings(0.50f)));
        writeWav(out / "MiniMix_DistantMetal80.wav", render(miniMix, distantMetalSettings(0.80f)));

        auto airSettings = [](float amount) {
            Settings a {};
            a.cold = 1.f;
            a.frost = 0.22f;
            a.atmosAType = 6.f / 9.f; // Air
            a.atmosAAmount = amount;
            return a;
        };

        writeWav(out / "WarmPad_Air50.wav", render(warmPad, airSettings(0.50f)));
        writeWav(out / "WarmPad_Air80.wav", render(warmPad, airSettings(0.80f)));
        writeWav(out / "Drums_Air50.wav", render(drums, airSettings(0.50f)));
        writeWav(out / "Drums_Air80.wav", render(drums, airSettings(0.80f)));

        auto rumbleSettings = [](float amount) {
            Settings r {};
            r.cold = 1.f;
            r.space = 0.18f;
            r.atmosAType = 3.f / 9.f; // Rumble
            r.atmosAAmount = amount;
            return r;
        };

        writeWav(out / "Drums_Rumble50.wav", render(drums, rumbleSettings(0.50f)));
        writeWav(out / "Drums_Rumble80.wav", render(drums, rumbleSettings(0.80f)));
        writeWav(out / "MiniMix_Rumble50.wav", render(miniMix, rumbleSettings(0.50f)));
        writeWav(out / "MiniMix_Rumble80.wav", render(miniMix, rumbleSettings(0.80f)));

        auto windSettings = [](float amount) {
            Settings v {};
            v.cold = 1.f;
            v.frost = 0.16f;
            v.atmosAType = 0.f / 9.f; // Wind
            v.atmosAAmount = amount;
            return v;
        };

        writeWav(out / "WarmPad_Wind50.wav", render(warmPad, windSettings(0.50f)));
        writeWav(out / "WarmPad_Wind80.wav", render(warmPad, windSettings(0.80f)));
        writeWav(out / "Keys_Wind50.wav", render(keys, windSettings(0.50f)));
        writeWav(out / "Keys_Wind80.wav", render(keys, windSettings(0.80f)));

        auto iceSettings = [](float amount, float material) {
            Settings i {};
            i.cold = 1.f;
            i.ice = amount;
            i.iceMaterial = material;
            return i;
        };

        writeWav(out / "Keys_IceCrystal50.wav", render(keys, iceSettings(0.50f, 0.f / 5.f)));
        writeWav(out / "Keys_IceShatter80.wav", render(keys, iceSettings(0.80f, 5.f / 5.f)));
        writeWav(out / "Drums_IceCrack50.wav", render(drums, iceSettings(0.50f, 2.f / 5.f)));
        writeWav(out / "Drums_IceShatter80.wav", render(drums, iceSettings(0.80f, 5.f / 5.f)));

        std::ofstream notes(out / "README.txt");
        notes <<
            "125A Colderator Listening Pack\n"
            "48 kHz / 16-bit stereo\n"
            "Each source: 6 s input + 4 s tail\n\n"
            "Stages:\n"
            "Dry: unprocessed reference\n"
            "Cold25: playable frost/cold coating\n"
            "Cold50: clear Frozen identity\n"
            "Cold75: cinematic transformation\n"
            "Cold100: Frozen Machine extreme\n\n"
            "Sources:\n"
            "WarmPad: soft harmonic sustained pad\n"
            "Keys: repeated warm chord stabs\n"
            "Drums: kick/snare/hat transient test\n"
            "MiniMix: pad + keys + drums + bass\n\n"
            "Targeted Frozen Bloom renders:\n"
            "WarmPad_FrozenBloom50/80: additive source-derived bloom checks\n"
            "Keys_FrozenBloom50/80: transient/harmonic bloom checks\n\n"
            "Targeted Machine renders:\n"
            "Drums_Machine50/80: mechanical load and impact checks\n"
            "MiniMix_Machine50/80: source-linked cinematic machine-room checks\n\n"
            "Targeted Distant Metal renders:\n"
            "Keys_DistantMetal50/80: sparse distant-strike checks\n"
            "MiniMix_DistantMetal50/80: cinematic distant-metal scene checks\n\n"
            "Targeted Air renders:\n"
            "WarmPad_Air50/80: thin frozen-air coating checks\n"
            "Drums_Air50/80: transient-linked cold-air checks\n\n"
            "Targeted Rumble renders:\n"
            "Drums_Rumble50/80: structural-impact and low-body checks\n"
            "MiniMix_Rumble50/80: source-linked cinematic rumble checks\n\n"
            "Targeted Wind renders:\n"
            "WarmPad_Wind50/80: broad real-wind layer checks\n"
            "Keys_Wind50/80: source-linked gust movement checks\n\n"
            "Targeted ICE renders:\n"
            "Keys_IceCrystal50: hard crystalline material check\n"
            "Keys_IceShatter80: extreme splintered material check\n"
            "Drums_IceCrack50: transient-driven physical shard check\n"
            "Drums_IceShatter80: extreme transient shard check\n";

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Listening render failed: " << e.what() << "\n";
        return 1;
    }
}
