# Colderator approved design assets

Status: APPROVED VISUAL SOURCE SET

This folder documents the approved Colderator faceplate and knob artwork before VSTGUI integration.

## Faceplate

Logical editor base:
- 100%: 800 x 560
- 150%: 1200 x 840

Prepared files:
- Colderator_Faceplate_master.png — 1600 x 1120
- Colderator_Faceplate_100.png — 800 x 560
- Colderator_Faceplate_150.png — 1200 x 840

Rules:
- no logo baked into the faceplate
- no text or labels baked into the faceplate
- no knobs or active controls baked into the faceplate
- no LEDs, values, scales or icons baked into the faceplate

## Knob body

One approved static knob body is used as the common visual source.

Prepared files:
- Colderator_Knob_Master.png — 1220 x 1220
- Colderator_Knob_main_100_120px.png — 120 x 120
- Colderator_Knob_main_150_180px.png — 180 x 180
- Colderator_Knob_character_100_88px.png — 88 x 88
- Colderator_Knob_character_150_132px.png — 132 x 132
- Colderator_Knob_utility_100_72px.png — 72 x 72
- Colderator_Knob_utility_150_108px.png — 108 x 108

Rules:
- static knob body only
- transparent background
- no baked-in pointer/index
- VSTGUI draws the pointer/index dynamically
- do not regenerate the knob style unless explicitly approved

## Integration order

1. Use the approved faceplate as the real background resource.
2. Add VSTGUI controls, labels and authoritative 125A branding separately.
3. Verify 100% and 150% layout/zoom.
4. Replace temporary controls with the approved static knob images.
5. Keep DSP, parameter IDs, state and automation semantics unchanged.

## Important

The currently committed experimental VSTGUI look is NOT the visual reference for further GUI work.
The approved faceplate and knob artwork described here are the new visual source of truth.

