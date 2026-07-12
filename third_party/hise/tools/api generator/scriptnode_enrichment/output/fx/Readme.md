---
title: FX Nodes
factory: fx
---

The fx factory contains audio effect nodes that apply transformations to the signal such as distortion, spatial processing, pitch shifting, and reverberation. These nodes process audio in place and are typically placed inside a [container.chain]($SN.container.chain$) or [container.split]($SN.container.split$) within a scriptnode network.

Most fx nodes are polyphonic and maintain per-voice state, with the exception of [fx.reverb]($SN.fx.reverb$) which operates as a shared monophonic processor. Several of these nodes produce output that benefits from combination with the dry signal via a parallel container - check each node's documentation for details.

## Nodes

| Node | Description |
|------|-------------|
| [$SN.fx.bitcrush$]($SN.fx.bitcrush$) | Bit depth reduction for lo-fi digital distortion |
| [$SN.fx.haas$]($SN.fx.haas$) | Stereo positioning using inter-channel delay (Haas effect) |
| [$SN.fx.phase_delay$]($SN.fx.phase_delay$) | First-order allpass filter for building comb filters and phasers |
| [$SN.fx.pitch_shift$]($SN.fx.pitch_shift$) | Real-time pitch shifting via resampling and time stretching |
| [$SN.fx.reverb$]($SN.fx.reverb$) | Freeverb-style algorithmic reverb (100% wet output) |
| [$SN.fx.sampleandhold$]($SN.fx.sampleandhold$) | Sample rate reduction by holding samples for configurable periods |
