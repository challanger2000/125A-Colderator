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

COLD is the main transformation macro.

It coordinates material, texture, movement and space. It is not a conventional wet/dry knob.

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
