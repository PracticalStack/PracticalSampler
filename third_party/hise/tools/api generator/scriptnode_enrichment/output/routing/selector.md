---
title: Selector
description: "Dynamically routes channels between selected positions and the front of the buffer."
factoryPath: routing.selector
factory: routing
polyphonic: true
tags: [routing, channels, selector, dynamic]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "routing.matrix", type: alternative, reason: "Arbitrary static channel routing via a visual matrix editor" }
commonMistakes:
  - title: "SelectOutput direction is not obvious"
    wrong: "Assuming SelectOutput controls whether the node is active"
    right: "SelectOutput controls the copy direction: Disabled copies from ChannelIndex to the front; Enabled copies from the front to ChannelIndex."
    explanation: "The parameter name is misleading. Think of it as choosing whether you are selecting which input channels to bring forward, or selecting which output position to send the front channels to."
llmRef: |
  routing.selector

  Dynamically routes channels within a multichannel buffer. Two modes controlled by SelectOutput: copy from a selected channel range to the front of the buffer, or copy from the front to a selected position. Optionally zeros non-selected channels.

  Signal flow:
    multichannel in -> channel copy (ChannelIndex, NumChannels, direction) -> optional clear -> multichannel out

  CPU: negligible, polyphonic

  Parameters:
    ChannelIndex: 0 - 16, step 1 (default 0). Source or destination channel offset.
    NumChannels: 1 - 16, step 1 (default 1). Number of consecutive channels to route.
    SelectOutput: Disabled / Enabled (default Disabled). Disabled = copy selected to front; Enabled = copy front to selected position.
    ClearOtherChannels: Disabled / Enabled (default Enabled). Whether to zero all channels outside the routed range.

  When to use:
    Rarely used (rank 103, 2 instances). For dynamic channel routing that can be modulated at runtime. For static routing, use routing.matrix instead.

  Common mistakes:
    SelectOutput controls copy direction, not activation.

  See also:
    [alternative] routing.matrix - static channel routing via visual matrix
---

Dynamically routes channels within a multichannel buffer. The node operates in two directions controlled by the SelectOutput parameter:

- **Disabled (default):** copies channels starting at ChannelIndex to the front of the buffer (channels 0 onwards). Use this to bring a selected channel range forward for processing.
- **Enabled:** copies the front channels to the position specified by ChannelIndex. Use this to place processed audio into a specific output position.

When ClearOtherChannels is enabled, all channels outside the routed range are zeroed. The ChannelIndex parameter supports per-voice modulation in polyphonic contexts, allowing different voices to select different channels.

For static channel routing that does not change at runtime, [routing.matrix]($SN.routing.matrix$) provides a visual matrix editor and is typically more convenient.

## Signal Path

::signal-path
---
glossary:
  parameters:
    ChannelIndex:
      desc: "Source or destination channel offset"
      range: "0 - 16"
      default: "0"
    NumChannels:
      desc: "Number of consecutive channels to route"
      range: "1 - 16"
      default: "1"
    SelectOutput:
      desc: "Copy direction: Disabled = selected to front, Enabled = front to selected"
      range: "Disabled / Enabled"
      default: "Disabled"
    ClearOtherChannels:
      desc: "Whether to zero non-routed channels"
      range: "Disabled / Enabled"
      default: "Enabled"
---

```
// routing.selector - dynamic channel router
// multichannel in -> multichannel out

process(channels) {
    if SelectOutput == Disabled:
        copy channels[ChannelIndex .. ChannelIndex + NumChannels] to channels[0 .. NumChannels]
    else:
        copy channels[0 .. NumChannels] to channels[ChannelIndex .. ChannelIndex + NumChannels]

    if ClearOtherChannels:
        zero all channels outside the routed range
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Routing
    params:
      - { name: ChannelIndex, desc: "The first channel to select from or route to. Can be modulated per voice in polyphonic contexts.", range: "0 - 16", default: "0" }
      - { name: NumChannels, desc: "Number of consecutive channels to route.", range: "1 - 16", default: "1" }
  - label: Configuration
    params:
      - { name: SelectOutput, desc: "Copy direction. Disabled copies from ChannelIndex to the front; Enabled copies from the front to ChannelIndex.", range: "Disabled / Enabled", default: "Disabled" }
      - { name: ClearOtherChannels, desc: "When enabled, all channels outside the routed range are set to silence.", range: "Disabled / Enabled", default: "Enabled" }
---
::

### Limitations

When ChannelIndex is 0 the copy step is skipped entirely -- only the clearing logic runs (if enabled). If the requested channel range exceeds the available channels, the node safely processes only the channels that exist.

**See also:** $SN.routing.matrix$ -- arbitrary static channel routing via a visual matrix editor
