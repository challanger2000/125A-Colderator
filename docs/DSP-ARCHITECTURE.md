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


## METAL identity pass

METAL now combines three material layers:

- source-coupled inharmonic resonators and sidebands for musical linkage;
- embedded real industrial-air material for steel/machine body;
- embedded real metallic events for large cinematic gestures;
- a controlled low-body mass layer so METAL can feel heavy rather than merely bright.

The six METAL materials use different balances of air, event and mass:
Steel / Pipe / Chain / Sheet / Machine / Rust.

Design target:
- at low settings the played source remains clearly identifiable;
- around 50% METAL should already feel like a distinct physical material;
- toward the upper range, large mechanical/industrial character may dominate.


## FROST identity pass

FROST is treated as a physical frozen surface rather than as generic digital grit.

Current layers:
- short held source fragments for local freezing/stiffness;
- deterministic micro-noise and crackle;
- embedded real ice/crack material chopped into tiny surface events;
- embedded real wind material high-passed into a cold air/snow skin;
- source-derived high-band body so the texture stays attached to the instrument.

Design target:
- low settings remain pitch-traceable and playable;
- around 50% the source should feel covered by a clearly frozen surface;
- the upper range may become rough, deep-frozen and strongly textural without becoming merely bitcrushed.


## SHIVER identity pass

SHIVER no longer represents wind or storm. Those scene-level behaviours belong to ATMOSPHERE.

SHIVER now models physical cold movement in the played material itself:

- Tremble: balanced micro-tremor.
- Stiff: slow, shallow movement as the source becomes rigid.
- Chatter: faster irregular tension.
- Strain: slower deeper bending and stress.
- Spasm: stronger episodic movement.
- Numb: minimal, sluggish movement near a frozen/stiff state.

Implementation:
- short moving delay / micro-Doppler;
- model-dependent motion rate and depth;
- irregular deterministic jitter;
- source-derived stress and rigidity terms;
- four-stage high-pass protection so the effect does not become a bass enhancer.

Design target:
- low settings remain clearly playable and pitch-traceable;
- around 50% SHIVER should feel like the instrument/material is physically reacting to cold;
- SHIVER must not duplicate WIND/STORM atmosphere.


## Frozen Landscape atmosphere

Frozen Landscape replaces the earlier Ghost atmosphere.

Purpose:
- provide an actual winter scene instead of another abstract modulation texture;
- combine real environmental material with source-derived musical body;
- remain silent when the input source is silent.

Layers:
- embedded real wind as wide cold air;
- embedded real industrial-air recording as distant frozen structure;
- very sparse embedded real ice-crack events;
- a low source-derived drone component;
- slow left/right horizon motion.

Frozen Landscape is available in both Atmosphere slots and is intentionally distinct from WIND and STORM: WIND is a weather texture, STORM is an active dramatic event, Frozen Landscape is a broad environmental scene.


## Frozen Bloom atmosphere

Frozen Bloom replaces the earlier generic Swell atmosphere.

Purpose:
- enlarge sustained source material without replacing or thinning the live source;
- create a slow, cold bloom behind pads/keys and other tonal material;
- remain fully source-derived and silent on fresh silence.

Mechanism:
- three delayed source-derived taps with channel-skewed timings;
- subtractive low-band filtering to keep the bloom cold rather than warm;
- dedicated source-activity envelope with sample-rate-invariant slow attack and long release;
- a small source-transient seed so short excitation can still feed the delayed bloom without generating sound from fresh silence;
- restrained feedback for a lingering bloom tail;
- additive mix behind the original signal rather than a crossfade.

Frozen Bloom is intended as a complementary large-scale texture, not a substitute for SPACE or STORM.


## Machine atmosphere identity pass

Machine is no longer a generic noise/carrier texture.

Purpose:
- create the impression of a large cold machine room reacting to the played source;
- remain distinct from the METAL module and from the sparse Distant Metal atmosphere;
- use real industrial source material for physical scale while keeping the scene dependent on input activity;
- remain silent on fresh silence.

Current layers:
- embedded real industrial-air recording as the continuous mechanical body;
- a source-derived low/mid load component so the machine follows the instrument rather than floating independently;
- a faster multi-cycle load pulse derived from the existing slow atmosphere phase;
- source-derived high-band stress;
- sparse embedded real metallic events for clanks/impacts;
- slight channel-asymmetric load movement for width without detached random panning.

Machine is a scene/bed generator. The METAL module remains the direct material transformation stage.


## Distant Metal atmosphere identity pass

Distant Metal is a sparse scene element, not a continuous machine bed and not a second METAL material mode.

Purpose:
- place occasional real metallic events far behind the source;
- preserve the impression of distance through damping and slower body energy;
- remain source-triggered and fully silent on fresh silence;
- stay clearly distinct from Machine, which is a continuous mechanical-room atmosphere.

Current layers:
- embedded real metallic-event recording as the strike source;
- low-pass body state to push the event away from the listener;
- a restrained synthetic ghost-resonance tail for scale without replacing the real strike;
- source transient/activity/swell coupling so events belong to the played material;
- slow scene motion for slight depth variation.

Distant Metal should feel like isolated metal activity somewhere in the frozen environment, while Machine should feel like the listener is inside an operating cold machine room.


## Air atmosphere identity pass

Air is now a thin freezing layer around the source rather than generic synthetic hiss.

Purpose:
- create the impression of cold moving air without duplicating the broader WIND scene;
- retain irregularity from a real field recording;
- couple the layer to source high-frequency/transient content;
- remain fully silent on fresh silence.

Current layers:
- embedded real wind recording with low body removed to isolate the colder air component;
- source-derived high-detail/breath component;
- a small deterministic high-noise component for frost-like edge;
- slow motion from the atmosphere phase;
- source-activity gate and swell so AIR follows the played material rather than running independently.

AIR should feel thin, close and freezing. WIND remains the broader weather texture.


## Rumble atmosphere identity pass

Rumble is now a source-coupled structural vibration rather than filtered low noise.

Purpose:
- add the feeling of heavy structural movement beneath the source;
- react to both sustained low-body energy and transients;
- remain distinct from Drone, which is sustained tonal body rather than mechanical/structural heave;
- remain silent on fresh silence.

Current layers:
- slowly tracked low-frequency source body;
- a separate transient/impact follower;
- low structural noise whose depth follows the tracked source body;
- direct low/mid source weight so the scene remains attached to the instrument;
- slow heave modulation from the atmosphere motion/gust states;
- source gate so no independent low-frequency bed appears from silence.

Rumble should feel like the floor, chassis or distant structure is moving with the sound, not like a continuous synthesized sub-noise layer.


## Wind atmosphere identity pass

Wind now uses the embedded real wind field recording as its physical basis instead of synthetic low-passed random noise.

Purpose:
- provide a broad natural frozen-wind layer;
- remain simpler and calmer than Frozen Storm;
- remain fuller and broader than the thin AIR layer;
- follow source activity so fresh silence remains silent.

Current layers:
- embedded real wind recording;
- a damped wind-body follower for broader mass;
- source-derived breath/high-detail component;
- gust shaping from the existing gust and slow-motion states;
- source gate to keep the atmosphere attached to the played material.

WIND is the basic broad weather layer. STORM adds pressure, snow, roar and cinematic surge. AIR is the thin high-frequency freezing surface.


## ICE identity pass

ICE now combines synthetic crystalline structure with short real ice/crack fragments instead of relying almost entirely on resonators, comb reflections and noise shards.

Purpose:
- preserve the precise hard/glassy character of the existing ICE DSP;
- add brief physical shard events so ICE feels like brittle material rather than only a synthetic resonator bank;
- remain clearly distinct from FROST, which is a continuous frozen-surface coating with crackle, grains and air;
- keep the real shard layer transient-driven and source-gated.

Current layers:
- short polarity-alternating glass reflections;
- source-coupled crystalline resonator modes;
- deterministic synthetic micro-shards;
- embedded real ice/crack fragments triggered by ICE events;
- per-material real-shard weighting: Crystal / Glass / Crack / Black Ice / Icicle / Shatter.

ICE should feel like hard brittle crystal breaking or ringing around the source. FROST should feel like a rough frozen surface covering the source.


## Core audit: macro neutrality, automation capacity and event-rate invariance

A post-identity-pass core audit found three correctness issues and defines the required behaviour:

- COLD is the scene director for Atmosphere as well as the foreground material modules. With COLD at 0%, Atmosphere A/B must contribute exactly no audible layer even when their Amount controls remain above zero.
- The processor exposes 17 automatable parameters. The realtime automation queue collector must therefore accept all 17 simultaneously; internal headroom is kept above the current public count so the last queue cannot be silently discarded.
- Probabilistic ICE, METAL, FROST and Atmosphere events are authored against a 48 kHz reference density. Per-sample probabilities are normalized by 48000/sampleRate so event density remains approximately constant per second across supported sample rates.

These are correctness constraints rather than new sound-design features.


## Core audit: sample-rate-invariant temporal states

Several smoothing states were historically expressed as fixed per-sample coefficients. Those values were authored at 48 kHz, so their physical time constants shortened as sample rate increased.

The affected states are now expressed through `onePoleCoeff(sampleRate, hz)` using the exact 48 kHz equivalent pole frequencies:
- SHIVER jitter smoothing;
- SHIVER wind-gust smoothing and filtered noise;
- Cinematic-depth disabled-state drain;
- Atmosphere low-noise smoothing;
- Atmosphere gust smoothing;
- Atmosphere swell smoothing.

This preserves the existing 48 kHz timing while making the intended temporal behaviour stable across sample rates.

The same audit removed confirmed unused local variables only; no user-facing parameter or state semantics changed.


## Host-compliance audit: silence flags, zero-sample flush and realtime overruns

The host/lifecycle audit adds three professional-behaviour requirements:

- Output `AudioBusBuffers::silenceFlags` are derived from the actual generated output on every processed block. This is necessary because a silent input can still produce a tail; input silence therefore cannot simply be mirrored during active processing.
- Zero-sample `process` calls are treated as parameter-flush calls and covered by regression tests so host-delivered parameter changes are not lost when no audio samples are present.
- Realtime QA now records deadline-overrun counts in addition to p95, p99, maximum callback time and the block deadline. The stress fixture includes both Atmosphere slots so the timed path represents a heavier actual product configuration.

No parameter IDs, state format or user-facing control semantics changed.


## Real-program regression corpus

The automated core QA now includes fixed real-program audio in addition to synthetic fixtures.

Current corpus:
- CC0 piano performance;
- CC0 drum loop;
- CC0 synth-pad production stem;
- CC0 spoken voice.

The fixture sources are not committed as binaries. CI downloads them from their documented source pages into a cache, prepares deterministic 48 kHz PCM16 excerpts without loudness normalization, and records source SHA-256 plus provenance. A dedicated CTest verifies neutral-path accuracy, finite/bounded output and a measurable transformation under a representative strong Colderator setting.
