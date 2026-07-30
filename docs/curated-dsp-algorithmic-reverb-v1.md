# Curated Algorithmic Reverb v1

`drs.algorithmicReverb` v1 is a deterministic, zero-latency feedback-delay-network reverb. It
uses a fixed four-line stereo FDN, a stereo pre-delay ring, damped feedback, and an orthogonal
Hadamard feedback mix. There are no convolution assets, external files, random modulation, or
tempo dependence. The topology and line-ratio family are part of version 1 and may only change
under a new effect version.

The stable v1 parameters are:

- `preDelayMs`: 0 to 250 ms before the FDN input.
- `size`: scales the fixed FDN line-ratio family without reallocating storage.
- `decaySeconds`: maps to a bounded feedback gain at the longest active line.
- `damping`: one-pole high-frequency loss inside each feedback line; zero is open and one is dark.
- `width`: scales the wet mid/side image.
- `mix`: equal-power-free linear wet/dry blend, matching the other curated v1 effects.

All delay storage is allocated during preparation and consumes 367,184 bytes at 96 kHz, below the
catalog's 512 KiB state request. The graph's 128-unit callback budget admits at most six 20-unit
reverb nodes. Reset and panic clear every ring and filter deterministically. Reverb tail
activity is capped at 30 seconds; a bounded retirement policy may end it earlier under activation
pressure. Parameters are smoothed by the graph control plane, and the reverb kernel interpolates
the resulting block endpoints per sample.
