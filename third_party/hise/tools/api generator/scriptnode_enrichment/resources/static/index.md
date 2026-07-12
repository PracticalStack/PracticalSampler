---
title: Scriptnode Reference
description: Reference for all scriptnode nodes - DSP building blocks for visual signal processing in HISE
---

Scriptnode is HISE's visual DSP framework. You build signal processing networks by connecting nodes - small, focused DSP operations - inside containers that define the signal flow. Each node has parameters that can be modulated, automated, or connected to other nodes via cables.

## Node Factories

Nodes are organised by factory. Each factory groups nodes with a common purpose.

| Factory | Nodes | Description |
|---------|-------|-------------|
| [analyse](/v2/reference/scriptnodes/analyse) | 4 | Display and analysis nodes (FFT, oscilloscope, goniometer) |
| [container](/v2/reference/scriptnodes/container) | 28 | Signal flow structures (chain, split, multi, branch, clone) |
| [control](/v2/reference/scriptnodes/control) | 47 | Control signal routing and transformation (PMA, modulators, cables) |
| [core](/v2/reference/scriptnodes/core) | 25 | Core DSP (oscillators, gain, delays, file players, modulators) |
| [dynamics](/v2/reference/scriptnodes/dynamics) | 5 | Dynamic range processing (compressor, limiter, gate) |
| [envelope](/v2/reference/scriptnodes/envelope) | 7 | Envelope generators and voice management |
| [filters](/v2/reference/scriptnodes/filters) | 10 | Filter processing (SVF, biquad, moog, ladder, convolution) |
| [fx](/v2/reference/scriptnodes/fx) | 6 | Audio effects (bitcrush, reverb, haas, pitch shift) |
| [jdsp](/v2/reference/scriptnodes/jdsp) | 7 | JUCE DSP wrappers (delay, chorus, compressor, panner) |
| [math](/v2/reference/scriptnodes/math) | 26 | Mathematical operations on audio signals |
| [routing](/v2/reference/scriptnodes/routing) | 14 | Signal routing (send/receive, global cables, matrix) |
| [template](/v2/reference/scriptnodes/template) | 15 | Pre-built composite nodes (dry/wet, mid/side, frequency splitters) |
