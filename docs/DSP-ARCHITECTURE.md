# 125A Colderator v0.1.0 — DSP Architecture

Status: active prototype architecture.

## Product objective

125A Colderator is a **Cold / Industrial / Cinematic Transformation FX**.

It is not primarily an EQ, exciter or subtle mix-finisher. The source is raw material.
The plugin transforms its perceived material, surface, movement, room and atmosphere so that
warm or familiar sources can become frozen, industrial, sterile, uncanny or hostile.

Reference images:
- warm concert guitar under a thick layer of ice;
- warm piano in an old cold mortuary / empty stone hall;
- frost, broken glass, steel pipes, winter wind, wet cold concrete;
- cemetery / church / bunker / refrigeration room atmosphere;
- "this makes me shiver" rather than merely "this has less low-mid warmth".

Music production and cinematic/game sound design are both first-class use cases.

## Controls and roles

### COLD — scene director

COLD is the main transformation-intensity macro.

It coordinates material, texture, movement and space. It is not a conventional wet/dry knob.
The secondary module controls are participation weights:
- module 0% = fully excluded, even when COLD is 100%;
- COLD 0% = neutral, even when modules are set above 0%;
- effective module strength is derived from COLD × module participation.

This lets the user deliberately exclude potentially intrusive characters such as SHIVER.

Target dramaturgy:
- 0–20%: cold tint, source clearly dominant;
- 20–40%: obvious Cold character;
- 40–60%: signature transformation; transformed material is already dominant;
- 60–80%: strong frozen / industrial scene;
- 80–100%: extreme Frozen Machine / cinematic transformation.

At 50% the user must not have to wonder whether the effect is active.

### ICE — crystal / broken glass / icicles

Percept:
- brittle, crystalline, glassy, splintering;
- short bright shards and hard reflections;
- cold material replacement, not simply more treble.

DSP families:
- short multi-tap glass / comb structures;
- transient-excited crystalline modes;
- polarity-alternating reflections;
- optional micro-pitch shards if later listening proves useful.

ICE at 50% should already sound like a real effect.

### METAL — steel / pipes / machinery

Percept:
- inharmonic, mechanical, hard, industrial;
- steel, pipes, chains, machine room;
- not a polite bell resonator.

DSP families:
- inharmonic modal resonances;
- FM/ring-mod-like sidebands;
- possible frequency-shift contribution if it improves identity.

METAL at 50% should be clearly industrial.
At 75–100% it may become strongly synthetic.

### FROST — hoarfrost / frozen surface / snow crunch

Percept:
- granular, rough, frozen surface;
- a layer that coats the source;
- not static white noise.

DSP families:
- deterministic micro-freeze / sample-hold;
- signal-derived grains;
- high-frequency texture driven by source energy;
- short discontinuous surface events.

FROST at 50% must create an obvious surface texture.

### SHIVER — wind / gusts / micro-Doppler / physical shivering

Percept:
- irregular cold movement;
- gusting, trembling, unstable;
- not a clean tremolo LFO.

DSP families:
- short modulated delay;
- irregular deterministic jitter;
- micro-Doppler / pitch-time motion;
- stereo-opposed movement where useful.

SHIVER at 50% must be audibly moving.

### SPACE — mortuary / refrigeration room / church / cold concrete

Percept:
- empty, hard, sterile, wet-cold, uncanny;
- early hard reflections and a lean cold tail;
- not a lush, warm hall.

DSP families:
- sparse early reflections;
- low-diffusion cold feedback tail;
- high-passed feedback to prevent warm bloom;
- metallic or stone-like reflection pattern;
- decorrelated but controlled stereo field.

SPACE is a scene generator, not only a reverb amount.

## Material-morph principle

The character controls increasingly **replace** the dry sonic material rather than merely adding
small parallel colour layers.

Working target:
- 20%: approximately 25% transformed material;
- 50%: approximately 70% transformed material;
- 75%: approximately 85–90% transformed material;
- 100%: transformed material may fully dominate.

These values are EMPIRICALLY TUNED prototype targets and must be validated by real listening.

## Source identity rule

The old requirement that ordinary tonal material remain conventionally playable through 80% is
superseded.

New rule:
- below ~30%, pitch/chord identity should remain easy to follow;
- around 50%, source identity may be strongly obscured but should remain traceable when musically useful;
- 75–100% may deliberately destroy conventional timbral identity;
- no module may collapse every input into the same accidental fixed pitch unless this is a deliberate
  extreme-region character.

The plugin is allowed to transform. It is not required to behave like a transparent insert.

## Measurement role

Measurement is a guardrail, not a reason to make the effect timid.

Measure:
- transformation strength at 25/50/75/100%;
- transient changes and time-domain texture;
- sidebands / modal energy / spectral distribution;
- residual source fundamentals where relevant;
- stereo correlation and motion;
- peak/RMS and level-match for comparisons;
- DC, NaN/Inf, denormals and boundedness;
- aliasing at 44.1/48/96 kHz for nonlinear/sideband stages;
- block-size / offline / realtime parity;
- p95/p99/max CPU and realtime deadlines.

A test must not call a module "audible" merely because a tiny numeric difference exists.
50% is the main signature working region.

## Nonlinear / aliasing rule

Intentional digital artifacts are allowed when they contribute to the Cold/Industrial identity.
Aliasing may be a designed texture, not automatically an error.

Any deliberate sideband or nonlinear stage must still be characterized for:
- harmonics and IMD;
- folded components below Nyquist;
- sample-rate dependence;
- DC and stability;
- level dependence.

No dedicated Aliasing control is planned.

## Realtime rule

Audio thread:
- no allocation;
- no locks;
- no logging/file/network I/O;
- bounded work;
- preallocated state;
- denormal protection.

## Evidence classes

Constants and mappings should be identified as:
- MEASURED
- DOCUMENTED
- CIRCUIT DERIVED
- PHYSICS DERIVED
- PUBLISHED-PARAMETER DERIVED
- EMPIRICALLY TUNED
- ESTIMATED / APPROXIMATED

The present artistic material mappings are EMPIRICALLY TUNED until real-audio listening and
measurement converge.


## Material model architecture

Each character module has six selectable material models. The selector changes the internal
DSP behaviour, not merely parameter presets.

- ICE: Crystal / Glass / Crack / Black Ice / Icicle / Shatter
- METAL: Steel / Pipe / Chain / Sheet / Machine / Rust
- FROST: Hoarfrost / Snow / Crunch / Frozen Dust / Rime / Deep Freeze
- SHIVER: Tremble / Wind / Gust / Storm / Whiteout / Polar
- SPACE: Morgue / Church / Bunker / Ice Cave / Cold Hall / Cemetery

The module amount remains the participation weight in the COLD transformation. The material
selector determines *how* that module sounds.

Current implementation differences include:
- distinct modal frequency sets for ICE and METAL;
- different event density, decay and particle balance;
- different FROST hold/crackle/noise behaviour;
- different SHIVER delay, jitter and wind/gust behaviour;
- different SPACE reflection geometry, damping and feedback.

Material selectors use stable parameter IDs 108–112.
Component state version 2 persists all five selections. Version-1 states remain loadable and
migrate to material index 0, preserving the original v0.1.0 sound as closely as possible.


## Cinematic depth layer

Colderator now contains an internal cinematic depth layer beneath the foreground material FX.

Purpose:
- add scale without turning every module into more reverb;
- restore controlled low-end weight where aggressive cold processing thins the source;
- create a distant rear-field cloud behind ICE/METAL/FROST/SHIVER;
- introduce slow motion over hundreds of milliseconds to seconds rather than only micro-modulation.

Current components:
- transient-coupled low-frequency weight around the source fundamental/low body;
- long stereo-skewed taps around 118 / 247 / 463 ms;
- a slower feedback path around 731 ms;
- slow bloom filtering;
- sub-Hz rear-field motion;
- internal draining when the cinematic layer is disabled so stale clouds cannot reappear.

The layer begins above the subtle COLD region and escalates toward high COLD values.
It remains dependent on module participation, so COLD with every module at 0% is still neutral.
SPACE remains a separate scene/room module; the cinematic layer is not a replacement for SPACE.


## Cinematic SPACE v2

SPACE now uses three perceptual depth layers instead of one short cold room:

- Near: sparse hard early reflections for walls, stone, metal and immediate location cues.
- Main: the existing cold low-diffusion feedback field.
- Far: a dedicated long field with approximately 173 / 389 / 713 ms taps and a ~1.127 s feedback path,
  stereo-skewed and scaled by the selected SPACE material.

The far layer has its own low-frequency rejection and slow bloom state. It fades in above the subtle
SPACE range and becomes increasingly important toward 100%.

At effective SPACE=100%, the direct foreground path remains fully removed; the output consists of the
wet Near/Main/Far scene only.

The host tail report is now 12 seconds to cover long cinematic decays and offline rendering safely.


## Frozen Storm identity pass

STORM is treated as a dedicated cinematic weather scene rather than a louder WIND mode.

Current layers:
- embedded CC0 real-wind bed for natural turbulence and irregularity;
- slow pressure body derived from the real field recording;
- macro-gust envelope that creates broad pressure surges;
- high-band snow/sleet texture that intensifies with gusts;
- slow stereo sweep for lateral weather movement;
- distant roar layer for large-scale mass.

At 50% atmosphere amount STORM is expected to be clearly distinguishable from WIND while remaining bounded.
