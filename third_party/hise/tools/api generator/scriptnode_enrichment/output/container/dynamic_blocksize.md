---
title: dynamic_blocksize
description: "Processes children with a parameter-controlled block size, allowing runtime quality settings down to per-sample frame processing."
factoryPath: container.dynamic_blocksize
factory: container
polyphonic: false
tags: [container, block-size, modulation-precision, quality-control]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors:
    - { parameter: "BlockSize", impact: "linear", note: "Smaller block sizes produce more chunk iterations per host buffer" }
seeAlso:
  - { id: "container.fix_blockx", type: alternative, reason: "Property-controlled block size for design-time evaluation" }
  - { id: "container.framex_block", type: companion, reason: "Equivalent to dynamic_blocksize with BlockSize set to 1" }
commonMistakes:
  - title: "Setting parameter to the actual block size"
    wrong: "Setting the BlockSize parameter to 64 expecting a 64-sample block size"
    right: "Set the parameter to the index: 4 for block size 64. The mapping is [1, 8, 16, 32, 64, 128, 256, 512] at indices 0-7."
    explanation: "The parameter value is an index into a fixed array of valid block sizes, not the block size itself. This ensures only valid power-of-two sizes are used."
llmRef: |
  container.dynamic_blocksize

  Serial container whose block size is controlled by a modulatable parameter. Splits audio into chunks of the selected size. When set to index 0 (block size 1), switches to per-sample frame processing.

  Signal flow:
    audio in -> BlockSize parameter selects chunk size -> children process each chunk/frame serially -> audio out

  CPU: negligible, monophonic. Smaller block sizes increase iteration count linearly.

  Parameters:
    BlockSize: 0 - 7, step 1 (default 4 = 64 samples). Index into [1, 8, 16, 32, 64, 128, 256, 512].

  When to use:
    When the end user should be able to choose a quality/CPU tradeoff at runtime, such as a "quality" knob in a synthesiser. Connect the parameter to a network parameter or control cable for automation.

  Common mistakes:
    Parameter is an index (0-7), not the block size itself. Index 4 = 64 samples.

  See also:
    [alternative] container.fix_blockx - property-controlled block size for design-time evaluation
    [companion] container.framex_block - equivalent to BlockSize index 0
---

The `dynamic_blocksize` container processes its children with a block size controlled by a modulatable parameter. Unlike [fix_blockx]($SN.container.fix_blockx$) where the block size is a design-time property, this node exposes it as a parameter that can be connected to network parameters or control cables. This makes it suitable for end-user "quality" settings that trade CPU cost for modulation precision at runtime.

When the BlockSize parameter is set to index 0 (block size 1), the container switches to per-sample frame processing, functionally equivalent to [framex_block]($SN.container.framex_block$). This means a single parameter can sweep the full range from frame-accurate processing down to coarse 512-sample chunks.

## Signal Path

::signal-path
---
glossary:
  parameters:
    BlockSize:
      desc: "Index selecting the chunk size: 0=1, 1=8, 2=16, 3=32, 4=64, 5=128, 6=256, 7=512"
      range: "0 - 7"
      default: "4 (64 samples)"
  functions:
    children.process:
      desc: "Processes all child nodes serially on the current chunk"
    children.processFrame:
      desc: "Processes all child nodes serially on a single sample frame"
---

```
// container.dynamic_blocksize - parameter-controlled block size
// audio in -> audio out

process(input) {
    if BlockSize == 1:
        // frame mode: per-sample processing
        for each sample:
            children.processFrame(sample)
    else:
        // block mode: chunked processing
        for each chunk of BlockSize samples in input:
            children.process(chunk)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Configuration
    params:
      - { name: BlockSize, desc: "Index into the block size table. Values map to block sizes [1, 8, 16, 32, 64, 128, 256, 512]. Fractional values are rounded to the nearest integer.", range: "0 - 7", default: "4 (64 samples)" }
---
::

### Block Size Table

Block sizes 2 and 4 are deliberately excluded from the table. These values would conflict with the internal event raster (default 8 samples), which could cause control-rate processing issues.

### Runtime Switching

When the block size changes at runtime, a brief silence (one buffer) may occur while children are re-prepared with the new block size. The audio thread is never blocked -- if re-preparation is in progress, the current buffer outputs silence instead. The compiled output retains the full block size dispatch, unlike [fix_blockx]($SN.container.fix_blockx$) which bakes a single static size. This adds one branch per buffer but preserves runtime flexibility.

When bypassed, children process the full host buffer without chunking. All other behaviours from the `fixN_block` family apply: MIDI events are timestamp-split across chunks, and the block size is a maximum (last chunk may be smaller).

**See also:** $SN.container.fix_blockx$ -- property-controlled block size for design-time evaluation, $SN.container.framex_block$ -- equivalent to BlockSize index 0
