---
title: CC Swapper
moduleId: CCSwapper
type: MidiProcessor
subtype: MidiProcessor
tags: [routing]
builderPath: b.MidiProcessors.CCSwapper
screenshot: /images/v2/reference/audio-modules/ccswapper.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso: []
commonMistakes:
  - title: "Identical CC values perform no swap"
    wrong: "Setting FirstCC and SecondCC to the same value expecting it to block that CC"
    right: "Use different values for FirstCC and SecondCC to perform a meaningful swap"
    explanation: "When both parameters are the same, matching CC events are 'swapped' to the same number - effectively a no-op. The module does not filter or block any events."
customEquivalent:
  approach: hisescript
  moduleType: ScriptProcessor
  complexity: trivial
  description: "A few lines in onController checking the CC number and calling Message.setControllerNumber()."
llmRef: |
  CCSwapper (MidiProcessor)

  Bidirectionally swaps two MIDI CC numbers. When a CC event arrives whose number matches FirstCC, the number is changed to SecondCC. When it matches SecondCC, the number is changed to FirstCC. All other MIDI events (notes, pitch bend, aftertouch, etc.) pass through unchanged. The CC value is preserved.

  Signal flow:
    MIDI in -> [if CC# == FirstCC: set to SecondCC; elif CC# == SecondCC: set to FirstCC] -> MIDI out

  CPU: negligible (two integer comparisons per CC event, event-rate only).

  Parameters:
    FirstCC (0 - 127, default 0) - first controller number in the swap pair
    SecondCC (0 - 127, default 0) - second controller number in the swap pair

  When to use:
    Remap CC assignments bidirectionally without scripting. Useful when a hardware controller sends on a fixed CC number that differs from the one a sound generator expects.

  Common mistakes:
    Both parameters default to 0 (Bank Select MSB). Set them to the desired CC numbers before use, or CC#0 messages will be affected.

  See also: (none)
---

::category-tags
---
tags:
  - { name: routing, desc: "Modules that forward, distribute, or proxy signals or events across the module tree." }
---
::

![CC Swapper screenshot](/images/v2/reference/audio-modules/ccswapper.png)

CC Swapper performs a bidirectional swap of two MIDI controller numbers. When a CC event arrives whose number matches one of the two configured values, the number is replaced with the other. All non-CC events and CC events that match neither parameter pass through unchanged. The CC value, channel, and all other event properties are preserved.

Both parameters default to 0, so CC#0 (Bank Select MSB) messages will be affected until the parameters are changed. When both parameters are set to the same value the module is effectively a no-op.

## Signal Path

::signal-path
---
glossary:
  parameters:
    FirstCC:
      desc: "First controller number in the swap pair"
      range: "0 - 127"
      default: "0"
    SecondCC:
      desc: "Second controller number in the swap pair"
      range: "0 - 127"
      default: "0"
---

```
// CC Swapper - bidirectional CC number swap
// MIDI in -> MIDI out

onMidiEvent(message) {
    if message is not CC:
        pass through unchanged

    if message.ccNumber == FirstCC:
        message.ccNumber = SecondCC
    else if message.ccNumber == SecondCC:
        message.ccNumber = FirstCC
    // else: pass through unchanged
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: CC Mapping
    params:
      - { name: FirstCC, desc: "First controller number in the swap pair. Incoming CC events with this number are changed to SecondCC.", range: "0 - 127", default: "0" }
      - { name: SecondCC, desc: "Second controller number in the swap pair. Incoming CC events with this number are changed to FirstCC.", range: "0 - 127", default: "0" }
---
::

### Bidirectional Swap

The swap is bidirectional: if FirstCC is 1 (Mod Wheel) and SecondCC is 11 (Expression), then CC#1 messages become CC#11 and CC#11 messages become CC#1.

### Unaffected Event Types

Pitch bend and aftertouch events are not affected, even though they are a type of controller message internally. Their controller numbers fall outside the 0-127 parameter range and can never match.
