---
title: Mid Side
description: "A mid/side processing template that decodes stereo to M/S, processes mid and side independently, then re-encodes to stereo."
factoryPath: template.mid_side
factory: template
polyphonic: false
tags: [template, stereo, mid-side, routing]
screenshot: /images/custom/scriptnode/ms_after.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "routing.ms_decode", type: companion, reason: "The M/S decoder used internally" }
  - { id: "routing.ms_encode", type: companion, reason: "The M/S encoder used internally" }
  - { id: "container.multi", type: companion, reason: "Splits the M/S signal into separate mono chains" }
commonMistakes:
  - title: "Using with mono input signal"
    wrong: "Feeding a mono signal into the mid/side template"
    right: "Ensure the input is stereo (2 channels). In a mono context, the side signal is always zero."
    explanation: "Mid/side encoding relies on the difference between left and right channels. With identical L and R (mono), the side channel is silent, making the side chain ineffective."
  - title: "Processing before decode or after encode"
    wrong: "Adding stereo processing inside the mid or side chains"
    right: "Each chain receives a mono signal. Use mono-compatible processing within the mid and side chains."
    explanation: "The container.multi splits the 2-channel M/S signal into two mono channels. Nodes inside mid_chain and side_chain each receive a single channel."
llmRef: |
  template.mid_side

  A composite template that provides a mid/side processing framework. Decodes stereo L/R to M/S, routes mid and side to independent mono processing chains, then re-encodes to stereo L/R.

  Signal flow:
    stereo L/R -> ms_decode -> container.multi -> mid_chain (ch0) + side_chain (ch1) -> ms_encode -> stereo L/R

  CPU: negligible, monophonic
    Adds only matrix encode/decode (2 additions per sample) and two gain multiplies. Actual cost depends on user processing.

  Parameters:
    No exposed top-level parameters.
    Internal: mid_gain (0.0 - 1.0, default 1.0), side_gain (0.0 - 1.0, default 1.0).

  When to use:
    Use for any effect that needs separate control over centre (mid) and stereo width (side) content. Common applications include stereo width control, mid/side EQ, and mid/side compression for mastering.

  Common mistakes:
    Requires stereo input. Each processing chain is mono.

  See also:
    [companion] routing.ms_decode -- the M/S decoder
    [companion] routing.ms_encode -- the M/S encoder
    [companion] container.multi -- splits M/S into mono chains
---

![Mid/side template](/images/custom/scriptnode/ms_after.png)

This template provides a ready-made mid/side processing framework. It decodes a stereo input into mid (centre) and side (stereo width) components using [ms_decode]($SN.routing.ms_decode$), routes each to an independent mono processing chain via a [multi container]($SN.container.multi$), then re-encodes the result to stereo with [ms_encode]($SN.routing.ms_encode$). The round-trip is unity gain when no processing is applied.

Both the mid_chain and side_chain contain placeholder gain nodes at unity (passthrough). Add your processing nodes inside these chains - for example, EQ, compression, or saturation applied independently to mid and side content. Adjust the internal `mid_gain` and `side_gain` values to control the balance between centre and stereo width, or expose them as macro parameters for top-level control.

## Signal Path

::signal-path
---
glossary:
  parameters:
    mid_gain:
      desc: "Gain multiplier for the mid (centre) signal (internal)"
      range: "0.0 - 1.0"
      default: "1.0"
    side_gain:
      desc: "Gain multiplier for the side (stereo width) signal (internal)"
      range: "0.0 - 1.0"
      default: "1.0"
  functions:
    ms_decode:
      desc: "Converts stereo L/R to mid/side: Mid = L+R, Side = L-R"
    ms_encode:
      desc: "Converts mid/side back to stereo: L = (M+S)/2, R = (M-S)/2"
---

```
// template.mid_side - mid/side processing framework
// stereo L/R in -> stereo L/R out

process(left, right) {
    // decode to mid/side
    mid, side = ms_decode(left, right)

    // independent mono processing
    mid = mid * mid_gain             // user adds processing here
    side = side * side_gain          // user adds processing here

    // re-encode to stereo
    left, right = ms_encode(mid, side)
}
```

::

### Limitations

This template requires a stereo (2-channel) input signal. With mono input the side channel is always zero, rendering the side chain ineffective. Each processing chain is mono -- use mono-compatible nodes inside mid_chain and side_chain.

### Gain Balance

Setting `side_gain` to 0 collapses the output to mono. Reducing it narrows the stereo image; increasing `mid_gain` relative to `side_gain` emphasises the centre content.

**See also:** $SN.routing.ms_decode$ -- the M/S decoder, $SN.routing.ms_encode$ -- the M/S encoder, $SN.container.multi$ -- splits M/S into mono chains
