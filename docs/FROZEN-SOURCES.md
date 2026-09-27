# Frozen Source Library

Colderator uses a hybrid design for selected cinematic textures:

- real field recordings provide natural irregularity and realism;
- DSP provides musical gating, motion, intensity, stereo treatment, filtering and integration with COLD.

Only sources with permissive reuse terms suitable for commercial distribution are admitted to the embedded library.

## Current embedded sources

### storm_wind
- Source: Wikimedia Commons — Howling wind.ogg
- Author: Tvabutzku1234
- License: CC0 1.0
- Use: Frozen Storm / Wind realism bed
- Build extraction: deterministic highest-RMS 10 s mono segment, resampled to 16 kHz, normalized and embedded as PCM.

### ice_crackle
- Source: Wikimedia Commons — Bones breaking wood fire ice crackling.ogg
- Author: stephan / PDSounds
- License: Public Domain
- Use: Ice Cracks event raw material
- Build extraction: deterministic highest-RMS 3 s mono segment, resampled to 16 kHz, normalized and embedded as PCM.

### cold_metal_air
- Source: Wikimedia Commons — Hammerschmiede Naichen 01.ogg
- Author: Flo Sorg
- License: CC0 1.0
- Use: reserved for Machine / industrial air development.
- Build extraction: deterministic highest-RMS 6 s mono segment.

### metal_chime
- Source: Wikimedia Commons — Windchimes.ogg
- Author: Esc861
- License: Public Domain
- Use: reserved for Distant Metal / glass-metal event development.
- Build extraction: deterministic highest-RMS 5 s mono segment.

Exact source pages and download endpoints are recorded in `assets/frozen_sources.json`.
The preparation script logs SHA-256 hashes for every downloaded source so release evidence can pin the exact bytes used.

## Packaging

The release artifact contains the prepared PCM data inside the VST3 binary. Users do not need a sample folder and the plugin performs no network or file I/O during audio processing.

Local/offline source builds use a zero-length fallback header unless `tools/prepare_frozen_sources.py` has been run.
