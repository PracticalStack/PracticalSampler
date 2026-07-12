---
title: FX Nodes
factory: fx
---

fx factory hold audio effect nodes. Transform signal: distortion, spatial, pitch shift, reverb. Process in place. Put inside [container.chain]($SN.container.chain$) or [container.split]($SN.container.split$).

Most fx polyphonic, per-voice state. Exception: [fx.reverb]($SN.fx.reverb$) shared monophonic. Several nodes benefit from dry mix via parallel container — check each doc.

## Nodes

| Node | Description |
|------|-------------|
| [$SN.fx.bitcrush$]($SN.fx.bitcrush$) | Bit depth reduction. Lo-fi digital distortion |
| [$SN.fx.haas$]($SN.fx.haas$) | Stereo placement via inter-channel delay (Haas) |
| [$SN.fx.phase_delay$]($SN.fx.phase_delay$) | First-order allpass. Build comb filters + phasers |
| [$SN.fx.pitch_shift$]($SN.fx.pitch_shift$) | Real-time pitch shift via resample + time stretch |
| [$SN.fx.reverb$]($SN.fx.reverb$) | Freeverb algorithmic reverb (100% wet) |
| [$SN.fx.sampleandhold$]($SN.fx.sampleandhold$) | Sample rate reduction by holding samples |
