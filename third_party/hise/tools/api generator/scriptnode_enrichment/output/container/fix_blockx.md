---
title: fix_blockx
description: "Splits the audio buffer into chunks with a property-selectable block size for evaluating different tradeoffs during development."
factoryPath: container.fix_blockx
factory: container
polyphonic: false
tags: [container, block-size, modulation-precision, development]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "container.fix8_block", type: alternative, reason: "Fixed 8-sample block size when the optimal size is already known" }
  - { id: "container.dynamic_blocksize", type: alternative, reason: "Parameter-controlled block size for end-user quality settings" }
commonMistakes:
  - title: "BlockSize property not visible"
    wrong: "Looking for the block size selector in the default node view"
    right: "Click the parameter button to reveal the BlockSize property."
    explanation: "The BlockSize property is hidden by default. The parameter button in the node header toggles visibility of node properties."
llmRef: |
  container.fix_blockx

  Serial container with a property-selectable block size. Splits the audio buffer into chunks of the chosen size. At compilation, the current property value is baked in as a static block size, eliminating the runtime dispatch.

  Signal flow:
    audio in -> split into N-sample chunks (N from property) -> children process each chunk serially -> audio out

  CPU: negligible, monophonic

  Parameters: none
  Properties: BlockSize (8, 16, 32, 64, 128, 256; default 64)

  When to use:
    During development to evaluate different block sizes without replacing the node. Once the optimal size is found, the compiled output is identical to using the corresponding fixN_block node directly.

  Common mistakes:
    BlockSize property is hidden by default - click the parameter button to reveal it.

  See also:
    [alternative] container.fix8_block - fixed 8-sample block size
    [alternative] container.dynamic_blocksize - parameter-controlled block size for end-user settings
---

The `fix_blockx` container splits the incoming audio buffer into chunks and processes its children serially on each chunk, just like the `fixN_block` family. The difference is that the block size is selected via a node property rather than being fixed by the node type. This makes it a development tool for evaluating the precision/CPU tradeoff: try different block sizes by changing the property, then compile to bake the chosen size into the output.

When compiled, the current BlockSize property value is used to generate a static block size path identical to the corresponding [fixN_block]($SN.container.fix8_block$) node. There is no runtime overhead from the property dispatch in the compiled output. During interpreted use in the HISE IDE, one additional branch per buffer selects the correct block size path - this is negligible.

## Signal Path

::signal-path
---
glossary:
  functions:
    children.process:
      desc: "Processes all child nodes serially on the current chunk"
---

```
// container.fix_blockx - property-selectable block size
// audio in -> audio out

process(input) {
    // BlockSize selected via node property (8-256)
    for each chunk of BlockSize samples in input:
        children.process(chunk)
}
```

::

### Setup

The BlockSize property is hidden by default -- click the parameter button in the node header to reveal it. Available values are 8, 16, 32, 64, 128, and 256. The default is 64. Changing the BlockSize property during playback triggers a full re-preparation of all children, which may cause a brief audio interruption. This is a design-time operation, not intended for real-time control. For runtime block size changes, use [dynamic_blocksize]($SN.container.dynamic_blocksize$) instead.

All behaviours from the `fixN_block` family apply: the block size is a maximum (last chunk may be smaller), MIDI events are timestamp-split across chunks, bypass reverts to full host buffer processing, and frame mode is preserved as a no-op.

**See also:** $SN.container.fix8_block$ -- fixed 8-sample block size, $SN.container.dynamic_blocksize$ -- parameter-controlled block size for end-user settings
