---
title: Routing Nodes
factory: routing
---

The routing factory provides nodes for directing audio signals and control values between different parts of a scriptnode network -- or across separate networks within the same HISE instance. Routing nodes fall into three categories: local signal routing (send/receive pairs within one network), global routing (cross-network audio and control value transport), and channel manipulation (mid-side encoding, channel selection, and arbitrary matrix routing).

Most routing nodes pass audio through without modification, acting as tap points or injection points in the signal graph. The exceptions are the channel manipulation nodes (ms_encode, ms_decode, selector, matrix), which actively rearrange or transform channel data.

## Nodes

| Node | Description |
|------|-------------|
| [$SN.routing.send$]($SN.routing.send$) | Copies audio to an internal buffer for retrieval by a paired receive node |
| [$SN.routing.receive$]($SN.routing.receive$) | Retrieves audio from a paired send node and mixes it into the signal path |
| [$SN.routing.global_cable$]($SN.routing.global_cable$) | Routes a normalised control value across DspNetworks via the global routing system |
| [$SN.routing.global_send$]($SN.routing.global_send$) | Sends audio to a named global slot for cross-network retrieval |
| [$SN.routing.global_receive$]($SN.routing.global_receive$) | Receives audio from a named global slot and mixes it into the signal path |
| [$SN.routing.local_cable$]($SN.routing.local_cable$) | Routes a normalised control value between nodes sharing the same ID within one network |
| [$SN.routing.local_cable_unscaled$]($SN.routing.local_cable_unscaled$) | Routes an unnormalised control value between nodes sharing the same ID within one network |
| [$SN.routing.matrix$]($SN.routing.matrix$) | Arbitrary channel routing matrix for remapping input channels to output channels |
| [$SN.routing.selector$]($SN.routing.selector$) | Dynamically selects a subset of channels for routing |
| [$SN.routing.ms_encode$]($SN.routing.ms_encode$) | Encodes a stereo signal from left-right to mid-side representation |
| [$SN.routing.ms_decode$]($SN.routing.ms_decode$) | Decodes a stereo signal from mid-side back to left-right representation |
| [$SN.routing.public_mod$]($SN.routing.public_mod$) | Exposes a control value as a modulation source for the parent network or compiled node |
| [$SN.routing.event_data_reader$]($SN.routing.event_data_reader$) | Reads per-event data from the global event storage |
| [$SN.routing.event_data_writer$]($SN.routing.event_data_writer$) | Writes per-event data to the global event storage |
