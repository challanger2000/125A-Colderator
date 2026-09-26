# 125A Colderator v0.1.0 — Measurement and QA Plan

Status: pre-implementation test specification.

## Purpose

The Colderator must prove that it creates a controllable perception of cold while remaining musically playable through the normal range.

Listening is required, but objective measurements must prevent:
- louder = better;
- thinner = colder;
- harsher = colder;
- fixed resonance = metallic;
- noise = frost.

## Fixed synthetic fixtures

1. Silence
2. Impulse
3. Unit step
4. 50 Hz, 100 Hz, 440 Hz, 1 kHz, 5 kHz sine
5. logarithmic sweep
6. two-tone IMD fixtures
7. pink noise
8. white noise
9. short transient bursts
10. synthetic harmonic tones with known fundamental
11. major/minor triads across several roots
12. chromatic note sequence

## Real-audio fixtures

Freeze a small corpus covering:
- clean electric guitar
- distorted/high-gain guitar
- bass
- kick/snare/drum loop
- cymbals
- synth lead
- synth pad
- piano/keys
- vocal/speech
- dense stereo mix

Do not regenerate fixtures simply to make regression pass.

## Neutrality tests

At:
- COLD=0
- ICE=0
- METAL=0
- FROST=0
- SHIVER=0
- SPACE=0
- OUTPUT=0 dB

Verify:
- sample-aligned null against input within floating-point tolerance;
- no latency unless explicitly introduced;
- no DC;
- no noise on silence;
- deterministic output;
- mono/stereo symmetry.

## Control-strength tests

For every character control measure at:
- 0%
- 10%
- 25%
- 50%
- 75%
- 90%
- 100%

Target behavior:
- 0% neutral;
- 25% clearly measurable and musically useful;
- 50% unambiguously audible;
- 75% strong;
- 90–100% deliberately extreme.

Document actual metric progression; do not assume linear perceptual scaling.

## Pitch/playability tests

### Sustained sine

For roots across the audible musical range:
- estimate dominant frequency before/after;
- verify no unrelated stable mode becomes dominant below the extreme region.

### Chromatic melody

Measure:
- fundamental tracking continuity;
- pitch error in cents where measurable;
- emergence of fixed modal peaks independent of input note.

### Chords

Use major/minor triads:
- inspect spectral peaks;
- listen for chord identity;
- ensure modal energy does not collapse all chords toward the same perceived resonance below 90%.

### Acceptance principle

0–80%:
- musical pitch/chord identity clearly retained.

90–100%:
- fixed/strong metallic resonances allowed as an intentional creative effect.

## ICE / METAL modal measurements

Per setting:
- modal peak frequencies;
- Q / bandwidth;
- decay time per mode;
- total modal energy relative to dry signal;
- peak-to-next-peak dominance;
- note-to-note variation;
- transient vs sustain excitation ratio.

Detect dangerous fixed-mode behavior by running multiple input fundamentals and comparing whether the same output peak dominates regardless of note.

## Spectral / perceptual proxies

Track:
- low-mid energy, especially broad regions around 150–500 Hz;
- spectral centroid;
- spectral rolloff;
- high-frequency energy;
- crest factor;
- transient peak ratio;
- roughness proxy where practical;
- band-limited RMS.

Do not treat any single metric as a quality score.

## Nonlinear measurements

If nonlinear processing is present:
- THD vs input level;
- harmonic spectrum;
- SMPTE/CCIF-style IMD where useful;
- alias power;
- DC offset;
- asymmetry;
- frequency dependence.

Run at:
- 44.1 kHz
- 48 kHz
- 96 kHz

Test targeted oversampling only if measured alias reduction justifies CPU/latency.

## FROST tests

Verify:
- silence does not create uncontrolled hiss;
- texture level follows input intentionally;
- deterministic renders for fixed seed/state;
- no excessive HF energy;
- no unacceptable alias-like components;
- no sudden texture bursts on parameter automation.

## SHIVER tests

Measure:
- pitch deviation in cents on sustained sine;
- amplitude modulation depth;
- modulation spectrum;
- stereo correlation;
- automation transitions.

Normal range must not sound like obvious vibrato/chorus unless deliberately designed.

## SPACE tests

Measure:
- impulse response;
- early-reflection times;
- tail length;
- spectral decay;
- inter-channel correlation;
- effective latency/tail reporting.

The normal range should remain sparse/cold rather than lush.

## Gain matching

Every subjective A/B must be level matched.

Record:
- integrated or fixture-appropriate RMS/LUFS delta;
- peak delta;
- crest-factor delta.

If an internal compensation stage is used, test that it reduces level bias without erasing intended dynamics.

## Automation tests

Automate each parameter:
- slow ramp;
- fast ramp;
- discontinuous host step;
- simultaneous multi-parameter moves.

Verify:
- no clicks beyond intentionally discontinuous creative behavior;
- no NaN/Inf;
- no runaway resonator state;
- smoothing does not prevent controls reaching exact end values.

## Realtime performance

Measure realistic plugin process blocks:
- mean
- p95
- p99
- max
- deadline overruns

Across:
- common block sizes
- supported sample rates
- mono/stereo
- idle/silence
- dense material
- extreme settings

## Release-stage host/QA matrix

Before release candidate:
- Steinberg VST3 Validator target 47/47 PASS
- 125A Plugin Tester PASS
- editor lifecycle
- activate/deactivate
- state save/restore
- bypass
- mono/stereo
- offline/realtime
- sample-rate changes
- block-size changes
- NaN/Inf
- denormal/subnormal stress
- deterministic regression
- control default reset
- automation/state synchronization

## Current evidence status

All DSP topology and thresholds in the early design are hypotheses until prototype measurements are recorded.
