---
title: Container Nodes
factory: container
---

Container nodes hold other nodes as children and control how they are processed. They define the structure of every scriptnode network - whether children run in series, in parallel, with modified sample rates, or under special conditions such as MIDI filtering or bypass smoothing.

The container structure follows a strict tree hierarchy. Each container modifies the processing context for its children: changing the signal routing, block size, sample rate, channel count, or event handling. Choosing the right container is the primary way to shape the architecture of a scriptnode network.

## Nodes

| Node | Description |
|------|-------------|
| [$SN.container.chain$]($SN.container.chain$) | A serial container that processes each child node in sequence |
| [$SN.container.split$]($SN.container.split$) | A parallel container that copies the input to each child and sums their outputs |
| [$SN.container.multi$]($SN.container.multi$) | A parallel container that assigns each child a different slice of the audio channels |
| [$SN.container.branch$]($SN.container.branch$) | A container that processes only the child selected by its Index parameter |
| [$SN.container.clone$]($SN.container.clone$) | An array of identical child node chains with configurable processing modes |
| [$SN.container.midichain$]($SN.container.midichain$) | A serial container that enables sample-accurate MIDI event processing |
| [$SN.container.modchain$]($SN.container.modchain$) | A serial container that processes children at control rate without affecting the parent audio |
| [$SN.container.sidechain$]($SN.container.sidechain$) | A serial container that doubles the channel count for sidechain routing |
| [$SN.container.no_midi$]($SN.container.no_midi$) | A serial container that blocks all MIDI events from reaching its children |
| [$SN.container.soft_bypass$]($SN.container.soft_bypass$) | A serial container with smoothed bypass crossfading to prevent clicks |
| [$SN.container.repitch$]($SN.container.repitch$) | Resamples audio before and after child processing to change the effective pitch |
| [$SN.container.offline$]($SN.container.offline$) | A container for offline processing that skips the realtime audio callback |
| [$SN.container.fix8_block$]($SN.container.fix8_block$) | Splits the audio signal into fixed-length chunks of 8 samples |
| [$SN.container.fix16_block$]($SN.container.fix16_block$) | Splits the audio signal into fixed-length chunks of 16 samples |
| [$SN.container.fix32_block$]($SN.container.fix32_block$) | Splits the audio signal into fixed-length chunks of 32 samples |
| [$SN.container.fix64_block$]($SN.container.fix64_block$) | Splits the audio signal into fixed-length chunks of 64 samples |
| [$SN.container.fix128_block$]($SN.container.fix128_block$) | Splits the audio signal into fixed-length chunks of 128 samples |
| [$SN.container.fix256_block$]($SN.container.fix256_block$) | Splits the audio signal into fixed-length chunks of 256 samples |
| [$SN.container.fix_blockx$]($SN.container.fix_blockx$) | Splits the audio signal into adjustable fixed-length chunks |
| [$SN.container.dynamic_blocksize$]($SN.container.dynamic_blocksize$) | Processes child nodes with a dynamic buffer size |
| [$SN.container.frame1_block$]($SN.container.frame1_block$) | Enables per-sample processing for 1 audio channel |
| [$SN.container.frame2_block$]($SN.container.frame2_block$) | Enables per-sample processing for 2 audio channels |
| [$SN.container.framex_block$]($SN.container.framex_block$) | Enables per-sample processing for the child nodes |
| [$SN.container.oversample$]($SN.container.oversample$) | Processes child nodes with a higher sample rate (configurable factor) |
| [$SN.container.oversample2x$]($SN.container.oversample2x$) | Processes child nodes at 2x the sample rate |
| [$SN.container.oversample4x$]($SN.container.oversample4x$) | Processes child nodes at 4x the sample rate |
| [$SN.container.oversample8x$]($SN.container.oversample8x$) | Processes child nodes at 8x the sample rate |
| [$SN.container.oversample16x$]($SN.container.oversample16x$) | Processes child nodes at 16x the sample rate |
