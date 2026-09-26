#pragma once

#include "base/source/fstreamer.h"

namespace Colderator {

constexpr Steinberg::int32 kComponentStateMagic = 0x31444C43; // "CLD1" little-endian
constexpr Steinberg::int32 kComponentStateVersion = 1;
constexpr int kComponentStateValueCount = 7;

inline bool readComponentStatePayload(Steinberg::IBStreamer& stream,
                                      float (&values)[kComponentStateValueCount],
                                      Steinberg::int32& bypass)
{
    Steinberg::int32 marker = 0;
    if (!stream.readInt32(marker) || marker != kComponentStateMagic)
        return false;

    Steinberg::int32 version = 0;
    if (!stream.readInt32(version) || version != kComponentStateVersion)
        return false;

    for (float& value : values)
        if (!stream.readFloat(value))
            return false;

    return stream.readInt32(bypass);
}

inline bool writeComponentStatePayload(Steinberg::IBStreamer& stream,
                                       const float (&values)[kComponentStateValueCount],
                                       Steinberg::int32 bypass)
{
    if (!stream.writeInt32(kComponentStateMagic)) return false;
    if (!stream.writeInt32(kComponentStateVersion)) return false;
    for (float value : values)
        if (!stream.writeFloat(value))
            return false;
    return stream.writeInt32(bypass);
}

} // namespace Colderator
