#pragma once

#include "base/source/fstreamer.h"

namespace Colderator {

constexpr Steinberg::int32 kComponentStateMagic = 0x31444C43; // "CLD1" little-endian
constexpr Steinberg::int32 kComponentStateVersion = 3;
constexpr int kComponentStateValueCount = 7;
constexpr int kComponentMaterialCount = 5;
constexpr int kComponentAtmosphereCount = 4;

struct ComponentStatePayload
{
    float values[kComponentStateValueCount] {};
    Steinberg::int32 bypass = 0;
    Steinberg::int32 materials[kComponentMaterialCount] {0, 0, 0, 0, 0};
    float atmosphere[kComponentAtmosphereCount] {0.f, 0.f, 0.f, 0.f};
};

inline bool readComponentStatePayload(Steinberg::IBStreamer& stream,
                                      ComponentStatePayload& payload)
{
    Steinberg::int32 marker = 0;
    if (!stream.readInt32(marker) || marker != kComponentStateMagic)
        return false;

    Steinberg::int32 version = 0;
    if (!stream.readInt32(version))
        return false;

    if (version < 1 || version > kComponentStateVersion)
        return false;

    for (float& value : payload.values)
        if (!stream.readFloat(value))
            return false;

    if (!stream.readInt32(payload.bypass))
        return false;

    if (version >= 2)
    {
        for (auto& material : payload.materials)
            if (!stream.readInt32(material))
                return false;
    }

    // Version 1/2 had no atmosphere slots. They migrate to Amount=0.
    if (version >= 3)
    {
        for (auto& value : payload.atmosphere)
            if (!stream.readFloat(value))
                return false;
    }

    return true;
}

inline bool writeComponentStatePayload(Steinberg::IBStreamer& stream,
                                       const ComponentStatePayload& payload)
{
    if (!stream.writeInt32(kComponentStateMagic)) return false;
    if (!stream.writeInt32(kComponentStateVersion)) return false;

    for (float value : payload.values)
        if (!stream.writeFloat(value))
            return false;

    if (!stream.writeInt32(payload.bypass)) return false;

    for (auto material : payload.materials)
        if (!stream.writeInt32(material))
            return false;

    for (float value : payload.atmosphere)
        if (!stream.writeFloat(value))
            return false;

    return true;
}

} // namespace Colderator
