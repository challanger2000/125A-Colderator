# 125A Colderator v0.1.0 — DSP Architecture

Status: design baseline for prototype implementation.

## Product objective

Translate physical/visual associations of cold into sound while keeping ordinary musical material playable through the normal working range.

Core perceptual targets:
- reduced warmth / reduced low-mid density without simply making the signal thin;
- harder, clearer transient edges;
- crystalline / glass-like upper structure;
- controllable inharmonic metallic character;
- fine frost-like roughness;
- subtle shiver-like micro-motion;
- sparse / distant cold spatial character.

The plugin must not reduce to a static EQ, exciter, bitcrusher, fixed resonator or ring modulator.

## User controls

Primary:
- COLD — global macro, 0..100%.

Secondary:
- ICE — crystalline / glass-like structure.
- METAL — inharmonic metallic structure.
- FROST — fine roughness / frozen surface texture.
- SHIVER — micro-motion.
- SPACE — cold spatial contribution.
- OUTPUT — post-trim.

Initial defaults:
- COLD 0%
- ICE 0%
- METAL 0%
- FROST 0%
- SHIVER 0%
- SPACE 0%
- OUTPUT 0 dB

0% on all character controls must be neutral.

## Control semantics

125A default scaling applies:
- 0% neutral/off
- 20–50% musical working range
- 50–75% clearly audible
- 75–100% strong/creative
- 100% meaningful maximum

Additional Colderator rule:
- values 0–80% should preserve clear pitch/chord identity on ordinary tonal material;
- 90–100% may intentionally become strongly resonant, metallic and effect-like.

No control may hide most of its useful range below 50%.

## Processing concept

### 1. Cold macro

COLD is not a wet/dry control. It is a coordinated perceptual macro.

Candidate sub-actions, to be measured independently before coupling:
1. reduce low-mid thermal density with broad dynamic/spectral shaping;
2. increase transient edge definition;
3. introduce controlled high-frequency crystalline structure;
4. reduce glue/smear while preserving loudness;
5. progressively expose ICE/METAL/FROST character according to their own user settings.

COLD=0 must null against bypass within numerical tolerance if OUTPUT=0 dB and all secondary controls are 0.

### 2. ICE engine

Goal:
- glass / icicle / crystal impression;
- no single fixed pitch in the normal range.

Candidate architecture:
- small bank of short-decay resonant modes;
- mode ratios intentionally inharmonic but distributed;
- excitation weighted toward transients and upper spectral content;
- frequencies may be weakly input-dependent or decorrelated to avoid one permanent resonant note;
- normal-range decay intentionally short to prevent modal takeover.

Extreme region:
- above roughly 90%, stronger Q and longer modal persistence are allowed.

### 3. METAL engine

Goal:
- frozen steel / cold machine surface rather than bell synthesis.

Candidate architecture:
- inharmonic modal bank with transient-driven excitation;
- optional very small frequency-shift / sideband component only if measurement and listening show added value;
- avoid fixed root-note resonator behavior below the extreme range;
- stereo channels remain coherent unless deliberate decorrelation is proven useful.

METAL at 50% must be unambiguously audible.
METAL at 100% may dominate and sound intentionally resonant/industrial.

### 4. FROST engine

Goal:
- fine frozen-surface roughness, not broadband hiss.

Candidate architecture:
- signal-dependent high-frequency texture;
- derive energy from input envelopes / spectral content;
- deterministic or seeded behavior for reproducible renders;
- avoid static noise at silence;
- mask-aware level limits should be investigated.

### 5. SHIVER engine

Goal:
- perceptual shivering without obvious chorus/vibrato in the musical range.

Candidate architecture:
- very low-depth bounded modulation of one or more of:
  - resonant mode positions;
  - gain;
  - spectral tilt;
  - micro-delay / phase;
- multiple low-amplitude rates may outperform one obvious LFO;
- preserve pitch accuracy on sustained sine tests through the normal range.

### 6. SPACE engine

Goal:
- sparse, empty, distinctly cold distance;
- include an intentionally icy reverb character, not only early reflections.

Candidate architecture:
- short sparse early reflections as the front edge;
- followed by a controlled cold reverb tail;
- deliberately lower diffusion than a conventional lush reverb;
- spectrally lean / glassy / metallic decay rather than warm dense bloom;
- decorrelated but stable stereo field;
- tail may be clearly audible and extended, but must remain sparse enough to preserve transients and musical pitch;
- avoid generic hall behavior, warm low-mid build-up and smooth analog-style glue.

The intended percept is an empty frozen space: reflective, cold, slightly metallic and clear rather than soft, enveloping or warm.

## Signal-flow hypothesis

Input
 -> analysis/envelopes
 -> cold spectral/dynamic shaping
 -> transient excitation extraction
 -> ICE/METAL parallel character paths
 -> FROST texture path
 -> SHIVER modulation applied to selected character parameters
 -> optional SPACE
 -> level compensation / safety
 -> OUTPUT

This is a hypothesis, not implementation authority. Each stage must earn its place through measurement and controlled listening.

## Pitch/playability rule

Mandatory:
- sustained sine input: normal-range settings must not impose a new dominant pitch;
- chromatic melody: detected fundamental progression must remain trackable;
- triads/chords: chord identity must remain recognizable;
- bass notes: no fixed modal frequency may dominate across different input notes.

A fixed resonant identity is acceptable only in the deliberate extreme 90–100% region.

## Nonlinear/aliasing rule

Any waveshaping or sideband-generating stage must be measured for:
- THD/harmonic spectrum;
- IMD;
- alias products at 44.1/48/96 kHz;
- level dependence;
- need for targeted oversampling.

Whole-plugin oversampling is not the default.

## Realtime rule

Audio thread:
- no allocation;
- no locks;
- no logging/file/network I/O;
- bounded work;
- preallocated state;
- denormal protection.

## Evidence classes

Every important constant / threshold / mapping must be marked as one of:
- MEASURED
- DOCUMENTED
- PHYSICS DERIVED
- PUBLISHED-PARAMETER DERIVED
- EMPIRICALLY TUNED
- ESTIMATED / APPROXIMATED

The prototype may start with EMPIRICALLY TUNED values, but measurements must replace assumptions before release claims.
