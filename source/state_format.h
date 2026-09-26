#pragma once

#include "base/source/fstreamer.h"

namespace Colderator {

constexpr Steinberg::int32 kComponentStateMagic = 0x31444C43; // "CLD1" little-endian
constexpr Steinberg::int32 kComponentStateVersion = 2;
constexpr int kComponentStateValueCount = 7;
constexpr int kComponentMaterialCount = 5;

struct ComponentStatePayload
{
    float values[kComponentStateValueCount] {};
    Steinberg::int32 bypass = 0;
    Steinberg::int32 materials[kComponentMaterialCount] {0, 0, 0, 0, 0};
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

    if (version != 1 && version != kComponentStateVersion)
        return false;

    for (float& value : payload.values)
        if (!stream.readFloat(value))
            return false;

    if (!stream.readInt32(payload.bypass))
        return false;

    // Version 1 had no material selectors. Material 0 reproduces the
    // original v0.1.0 behavior and is therefore the migration default.
    if (version >= 2)
    {
        for (auto& material : payload.materials)
            if (!stream.readInt32(material))
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

    return true;
}

} // namespace Colderator
