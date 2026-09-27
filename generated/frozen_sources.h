#pragma once
#include <cstddef>
#include <cstdint>

namespace Colderator::FrozenSources {

// Fallback for local/offline builds. CI/release builds overwrite this file
// with deterministic embedded PCM extracted from documented CC0/PD sources.
inline constexpr int kSampleRate = 16000;

inline constexpr std::int16_t k_storm_wind[] = {0};
inline constexpr std::size_t k_storm_wind_count = 0;

inline constexpr std::int16_t k_ice_crackle[] = {0};
inline constexpr std::size_t k_ice_crackle_count = 0;

inline constexpr std::int16_t k_cold_metal_air[] = {0};
inline constexpr std::size_t k_cold_metal_air_count = 0;

inline constexpr std::int16_t k_metal_chime[] = {0};
inline constexpr std::size_t k_metal_chime_count = 0;

} // namespace Colderator::FrozenSources
