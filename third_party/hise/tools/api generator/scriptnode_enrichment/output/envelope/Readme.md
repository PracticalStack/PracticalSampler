---
title: Envelope Nodes
description: "The envelope factory contains nodes for amplitude shaping, voice lifecycle management, and gate signal generation."
factory: envelope
---

The envelope factory contains nodes for shaping audio amplitude over time, managing voice lifecycle, and generating gate signals from modulation sources. Envelope nodes respond to MIDI note-on/off events to trigger their stages and provide modulation outputs that drive downstream voice management.

Most envelope nodes produce two modulation outputs: a continuous CV signal tracking the envelope value (0 to 1) and a binary Gate signal indicating whether the voice is still active. The Gate output is typically connected to an [envelope.voice_manager]($SN.envelope.voice_manager$) node, which sends the voice reset message that actually stops the voice from being rendered.

## Nodes

| Node | Description |
|------|-------------|
| [$SN.envelope.ahdsr$]($SN.envelope.ahdsr$) | Standard AHDSR envelope with attack curve shaping, manual gate, and retrigger support. |
| [$SN.envelope.flex_ahdsr$]($SN.envelope.flex_ahdsr$) | Advanced AHDSR with per-segment curve control, trigger/note/loop modes, and a draggable graph UI. |
| [$SN.envelope.simple_ar$]($SN.envelope.simple_ar$) | Lightweight attack-release envelope with a fixed sustain at full level. Lowest CPU cost. |
| [$SN.envelope.silent_killer$]($SN.envelope.silent_killer$) | Sends a voice reset when silence is detected after note-off. Fallback voice cleanup. |
| [$SN.envelope.voice_manager$]($SN.envelope.voice_manager$) | Sends a voice reset when the input value drops below 0.5. Standard gate-based voice lifecycle control. |
| [$SN.envelope.global_mod_gate$]($SN.envelope.global_mod_gate$) | Binary gate output reflecting whether a global modulator is still active for the current voice. |
| [$SN.envelope.extra_mod_gate$]($SN.envelope.extra_mod_gate$) | Binary gate output reflecting whether an extra modulation chain is still active for the current voice. |
