---
title: Specs
description: "A debug tool that displays the current processing context: sample rate, block size, channel count, and MIDI/polyphony status."
factoryPath: analyse.specs
factory: analyse
polyphonic: false
tags: [analyse, specs, debug, utility]
screenshot: /images/v2/reference/scriptnodes/analyse/specs.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "analyse.fft", type: companion, reason: "Often used alongside specs to verify signal context during development" }
commonMistakes:
  - title: "Removed during C++ export"
    wrong: "Relying on the specs node to be present in a compiled plugin"
    right: "Use specs only as a design-time debugging aid. It is automatically removed when the network is compiled to C++."
    explanation: "The node has the UncompileableNode flag set, meaning it is stripped from the network during C++ source export. This is intentional -- it is a development tool with no effect on audio processing."
llmRef: |
  analyse.specs

  Debug utility that displays processing context information: sample rate, block size, channel count, MIDI processing status, and polyphony state. Audio passes through unmodified. No display buffer, no parameters. Removed during C++ export (UncompileableNode).

  Signal flow:
    audio in -> audio out (unchanged)
    (side effect: UI displays processing specs)

  CPU: negligible, monophonic

  Parameters:
    None.

  When to use:
    Debugging processing context at any point in the signal chain. Place multiple instances at different locations to compare specs before and after containers that modify block size, channel count, or MIDI routing. Not observed in surveyed networks (design-time only).

  Common mistakes:
    Removed during C++ export -- design-time tool only.

  See also:
    [companion] analyse.fft - often used alongside to verify signal context
---

![Specs screenshot](/images/scriptnode/specs.png)

Displays the audio processing specifications at the current location in the signal chain. The node shows:

- **Sample rate** in Hz
- **Block size** in samples
- **Channel count**
- **MIDI processing** status (whether MIDI events are routed to this point)
- **Polyphony** status (whether the network is running in polyphonic mode)

Audio passes through completely unmodified. The node has no parameters, no display buffer, and no effect on the signal. It exists purely as a design-time inspection tool.

### Setup

Place multiple instances at different points in the chain to compare how containers affect the processing context. For example, placing one before and one after a [container.fix32_block]($SN.container.fix32_block$) reveals the block size change, or before and after a [container.no_midi]($SN.container.no_midi$) shows the MIDI routing change.

> [!Warning:Removed during C++ export] This node is automatically stripped when the network is compiled to a C++ class. Do not rely on it being present in an exported plugin.

**See also:** $SN.analyse.fft$ -- often used alongside to verify signal context during development
