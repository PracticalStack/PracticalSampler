---
title: Split
description: "A parallel container that copies the input to each child and sums their outputs."
factoryPath: container.split
factory: container
polyphonic: false
tags: [container, parallel]
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors:
    - { parameter: "child count", impact: "linear", note: "Each additional child adds a buffer copy and add operation" }
seeAlso:
  - { id: "container.chain", type: alternative, reason: "Serial processing instead of parallel" }
  - { id: "container.multi", type: alternative, reason: "Channel splitting instead of signal copying" }
  - { id: "container.clone", type: alternative, reason: "Parallel processing with identical children" }
commonMistakes:
  - title: "Unexpected gain increase from summing"
    wrong: "Adding multiple children to a split without compensating output level"
    right: "Add a core.gain node after the split or inside each child to compensate for the summed output."
    explanation: "Split sums all child outputs together. Two children at unity gain produce twice the amplitude. Scale each child or the combined output to maintain the expected level."
llmRef: |
  container.split

  A parallel container that copies the input signal to each child node and sums their outputs. Each child processes independently - they do not see each other's modifications.

  Signal flow:
    input --copy--> child[0] --\
    input --copy--> child[1] ---+--> sum --> output
    input --copy--> child[N] --/

  CPU: low, monophonic
    Buffer copy on entry; one copy + add per child beyond the first.

  Parameters:
    None

  When to use:
    Parallel effect chains, dry/wet mixing, multiband processing, any topology where the same input feeds multiple independent processing paths whose results are combined.

  Common mistakes:
    Summing multiple children at unity gain increases the output level. Compensate with gain staging.

  See also:
    [alternative] container.chain -- serial processing instead of parallel
    [alternative] container.multi -- channel splitting instead of signal copying
    [alternative] container.clone -- parallel processing with identical children
---

The split container creates parallel signal paths. Each child receives a copy of the original input, processes it independently, and all outputs are summed together. This is the standard way to build dry/wet mixers, parallel effect chains, and multiband processors.

Bypassed children are skipped - they contribute nothing to the output sum. When the entire split is bypassed, audio passes through unmodified. With only a single child, no buffer copies are made and the container behaves identically to a chain.

## Signal Path

::signal-path
---
glossary:
  functions:
    copy input:
      desc: "Saves a copy of the original input before child processing"
    sum outputs:
      desc: "Adds each child's output to the combined result"
---

```
// container.split - parallel processing with output summing
// audio in -> audio out

dispatch(input) {
    original = copy input
    child[0].process(input)             // first child: in-place
    for each remaining child:
        work = copy original
        child[i].process(work)
        sum outputs(input, work)        // add to result
}
```

::

Each child receives an independent copy of the original input, so changes made by one child do not affect other children. Memory cost is constant regardless of child count since a single work buffer is reused for each additional child.

**See also:** $SN.container.chain$ -- serial processing instead of parallel, $SN.container.multi$ -- channel splitting instead of signal copying, $SN.container.clone$ -- parallel processing with identical children
