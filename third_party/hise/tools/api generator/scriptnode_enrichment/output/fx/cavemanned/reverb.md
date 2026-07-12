---
title: Reverb
description: "A Freeverb-style algorithmic reverb with room size, damping, and width controls. Outputs 100% wet signal."
factoryPath: fx.reverb
factory: fx
polyphonic: false
tags: [fx, reverb, spatial]
screenshot: /images/v2/reference/scriptnodes/fx/reverb.png
cpuProfile:
  baseline: medium
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "filters.convolution", type: alternative, reason: "Impulse response reverb for realistic spaces versus algorithmic reverb" }
  - { id: "SimpleReverb", type: module, reason: "Direct equivalent -- Freeverb algorithm" }
commonMistakes:
  - title: "Output is 100% wet only"
    wrong: "Using fx.reverb inline expecting a dry/wet mix"
    right: "Place fx.reverb inside a container.split or use a dry_wet template to mix the wet reverb output with the dry signal."
    explanation: "The node outputs only the wet reverb signal with no dry component. For a blended reverb, use a parallel container to sum the dry and wet paths."
llmRef: |
  fx.reverb

  Freeverb-style algorithmic reverb outputting 100% wet signal. Suitable for adding room ambience and tail to audio signals.

  Signal flow:
    audio in -> 8 parallel comb filters + 4 series allpass filters (100% wet) -> audio out

  CPU: medium, monophonic

  Parameters:
    Damping (0.0 - 1.0, default 0.5) - high-frequency absorption in the reverb tail. Higher values produce a darker, more absorbed sound.
    Width (0.0 - 1.0, default 0.5) - stereo width of the reverb output. Has limited effect on the output.
    Size (0.0 - 1.0, default 0.5) - room size controlling the length of the reverb tail. Higher values produce longer decay.

  When to use:
    Adding algorithmic reverb within a scriptnode network. Place inside a container.split for dry/wet control. For realistic space simulation, consider filters.convolution instead.

  Common mistakes:
    Output is 100% wet - use a parallel container for dry/wet mixing.
    Room Size has no simple mapping to decay time in seconds - depends on Width and Damping too.

  Stereo:
    Always spreads input across the full stereo field (expected diffusion behaviour, not a bug).

  See also:
    alternative filters.convolution - impulse response reverb
    [module] SimpleReverb - direct equivalent -- Freeverb algorithm
---

Freeverb algorithmic reverb. Input → parallel comb filters → series allpass → diffuse tail. Outputs 100% wet — blend with dry by putting inside [container.split]($SN.container.split$) or dry/wet template.

Monophonic — single shared reverb, no per-voice state. Typical bus effect pattern. Handles mono + stereo auto. Mono in → mono out. Stereo in → decorrelate channels for wider image.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Size:
      desc: "Room size controlling reverb tail length"
      range: "0.0 - 1.0"
      default: "0.5"
    Damping:
      desc: "High-frequency absorption in the reverb tail"
      range: "0.0 - 1.0"
      default: "0.5"
    Width:
      desc: "Stereo spread of the reverb output"
      range: "0.0 - 1.0"
      default: "0.5"
  functions:
    combFilters:
      desc: "Eight parallel comb filters that create the initial reverb density"
    allpassFilters:
      desc: "Four series allpass filters that diffuse the comb filter output"
---

```
// fx.reverb - algorithmic reverb (100% wet)
// audio in -> audio out

process(input) {
    // Freeverb topology
    early = combFilters(input, Size, Damping)     // 8 parallel comb filters
    output = allpassFilters(early)                  // 4 series allpass filters
    // Width controls stereo decorrelation
    // Output is 100% wet (no dry signal mixed in)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Reverb
    params:
      - { name: Size, desc: "Room size. Controls the feedback amount in the comb filters, determining the length of the reverb tail. Higher values produce longer, more spacious decay. There is no simple mapping to decay time in seconds -- the actual decay also depends on Width and Damping.", range: "0.0 - 1.0", default: "0.5" }
      - { name: Damping, desc: "High-frequency absorption. Higher values cause high frequencies to decay faster than low frequencies, producing a warmer, darker reverb tail.", range: "0.0 - 1.0", default: "0.5" }
  - label: Stereo
    params:
      - { name: Width, desc: "Stereo width of the reverb output. Controls the decorrelation between left and right channels. Has limited effect on the output.", range: "0.0 - 1.0", default: "0.5" }
---
::

### Dry/Wet Control

Dry/wet: put node inside [container.split]($SN.container.split$) alongside dry path. Use [control.xfader]($SN.control.xfader$) to blend.

### Stereo Behaviour

Reverb always makes full stereo out from any input, hard-panned mono too. Expected diffusion — hard-panned signal spreads reverb across stereo field. Intentional, not bug.

**See also:** $SN.filters.convolution$ -- impulse response reverb for realistic acoustic spaces, $MODULES.SimpleReverb$ -- direct equivalent -- Freeverb algorithm
