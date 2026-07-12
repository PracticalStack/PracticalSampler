---
title: Dynamics Nodes
factory: dynamics
---

The dynamics factory provides nodes for controlling the dynamic range of audio signals. This includes standard downward compression, gating, peak limiting, envelope following, and dual-threshold compression with both upward and downward processing. All dynamics processors (except the envelope follower) are monophonic and operate on stereo or mono frame-based signals.

Most nodes in this factory output a normalised modulation signal representing gain reduction or envelope level, which can be routed to other parameters for metering, visualisation, or dynamics-linked effects. The compressor, gate, and limiter share the same parameter set and support external sidechain keying via extra channels.

## Nodes

| Node | Description |
|------|-------------|
| [$SN.dynamics.comp$]($SN.dynamics.comp$) | Downward compressor with peak detection and sidechain support |
| [$SN.dynamics.gate$]($SN.dynamics.gate$) | Noise gate that attenuates signals below the threshold |
| [$SN.dynamics.limiter$]($SN.dynamics.limiter$) | Peak limiter with fast envelope detection |
| [$SN.dynamics.envelope_follower$]($SN.dynamics.envelope_follower$) | Polyphonic envelope follower for amplitude tracking |
| [$SN.dynamics.updown_comp$]($SN.dynamics.updown_comp$) | Dual-threshold compressor with upward and downward compression |
