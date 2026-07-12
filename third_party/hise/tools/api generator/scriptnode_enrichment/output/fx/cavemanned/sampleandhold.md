---
title: Sample and Hold
description: "Reduces the effective sample rate by holding each captured sample for a configurable number of periods."
factoryPath: fx.sampleandhold
factory: fx
polyphonic: true
tags: [fx, lo-fi, distortion]
screenshot: /images/v2/reference/scriptnodes/fx/sampleandhold.png
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors:
    - { parameter: Counter, impact: low, note: "Low Counter values use per-sample processing; high values use optimised block fill" }
seeAlso:
  - { id: "fx.bitcrush", type: companion, reason: "Often paired for combined lo-fi effects - bitcrush reduces amplitude resolution, sampleandhold reduces time resolution" }
commonMistakes:
  - title: "Counter of 1 is a pass-through"
    wrong: "Setting Counter to 1 expecting any audible effect"
    right: "Counter=1 captures every sample, producing no change. Use values of 2 or higher for audible decimation."
    explanation: "At Counter=1 every input sample is captured and output unchanged. The effect only becomes audible at Counter=2 and above."
llmRef: |
  fx.sampleandhold

  Reduces the effective sample rate by holding each captured sample for a set number of periods. Produces the staircase waveform characteristic of low sample rate audio.

  Signal flow:
    audio in -> capture sample every N periods -> hold value for N-1 periods -> audio out

  CPU: negligible, polyphonic

  Parameters:
    Counter (1 - 64, integer, default 1) - number of samples to hold each captured value. 1 = pass-through, 64 = maximum decimation.

  When to use:
    Lo-fi effects, sample rate reduction, retro digital character. Pair with fx.bitcrush for combined bit-depth and sample-rate reduction.

  Common mistakes:
    Counter=1 is pass-through, not an effect.

  See also:
    companion fx.bitcrush - bit-depth reduction counterpart
---

Sample rate reduction. Capture one input sample, hold value N periods. Result = staircase waveform. Cuts effective time resolution. Lo-fi sound of low sample rate audio.

Each voice own counter + held value — polyphonic does not share decimation state. Counter=1 = pass-through. Higher Counter = fewer captures, stronger decimation. Counter=64 = every 64th sample captured, effective rate ÷64. When Counter large enough that whole block fits single hold period, node uses optimised block fill instead of per-sample.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Counter:
      desc: "Number of samples to hold each captured value"
      range: "1 - 64"
      default: "1"
  functions:
    hold:
      desc: "Stores the current sample and outputs it for the next N-1 periods"
---

```
// fx.sampleandhold - sample rate reduction
// audio in -> audio out

process(input) {
    for each sample:
        if counter == 0:
            heldValue = hold(sample)
            counter = Counter
        else:
            sample = heldValue
            counter -= 1
}
```

::

## Parameters

::parameter-table
---
groups:
  - label:
    params:
      - { name: Counter, desc: "Number of samples to hold each captured value. At 1, every sample passes through unchanged. Higher values produce stronger decimation with a more pronounced staircase effect.", range: "1 - 64", default: "1" }
---
::

**See also:** $SN.fx.bitcrush$ -- bit-depth reduction counterpart, pair for combined lo-fi
