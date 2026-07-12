---
title: Dry Wet
description: "A dry/wet parallel mixer template with linear crossfade between unprocessed and processed signal paths."
factoryPath: template.dry_wet
factory: template
polyphonic: false
tags: [template, mixing, parallel]
screenshot: /images/custom/scriptnode/drywet.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "container.split", type: companion, reason: "The underlying parallel container used internally" }
  - { id: "control.xfader", type: companion, reason: "The crossfade controller used internally" }
commonMistakes:
  - title: "Removing wet_gain from the wet path"
    wrong: "Deleting the wet_gain node when adding custom processing to the wet path"
    right: "Keep wet_gain at the end of the wet path. Add or replace nodes before it."
    explanation: "The wet_gain node applies the crossfade level to the wet signal. Without it, the wet path is always at full volume regardless of the DryWet setting."
  - title: "Expecting equal-power crossfade"
    wrong: "Assuming constant loudness at all DryWet positions with uncorrelated signals"
    right: "The crossfade is linear, not equal-power. Expect a slight level dip at the midpoint for uncorrelated dry and wet signals."
    explanation: "Linear crossfade sums amplitudes to 1.0 at all positions, but uncorrelated signals lose roughly 3 dB of perceived loudness at the midpoint compared to the extremes."
llmRef: |
  template.dry_wet

  A composite template that provides a dry/wet parallel mixer with a single crossfade parameter. Copies the input to two parallel paths (dry and wet), applies complementary gain, and sums the result.

  Signal flow:
    input --copy--> dry_path (dry_gain) --\
    input --copy--> wet_path (user processing -> wet_gain) ---+--> sum --> output
    DryWet -> xfader -> dry_gain (1-x), wet_gain (x)

  CPU: negligible, monophonic
    Adds only a split, an xfader, and two gain nodes. Actual cost depends on wet path content.

  Parameters:
    DryWet (0.0 - 1.0, default 0.0): Crossfade balance. 0 = fully dry, 1 = fully wet.

  When to use:
    Use whenever an effect needs a dry/wet mix control. Drop in the template, add your effect processing to the wet path, and expose the DryWet parameter.

  Common mistakes:
    Do not remove wet_gain from the wet path. The crossfade is linear, not equal-power.

  See also:
    [companion] container.split -- the underlying parallel container
    [companion] control.xfader -- the crossfade controller
---

![Dry/wet template](/images/custom/scriptnode/drywet.png)

This template provides a ready-made dry/wet mixer. It uses a [split container]($SN.container.split$) to duplicate the input into two parallel paths, applies a linear crossfade via an [xfader]($SN.control.xfader$), and sums the results. At DryWet = 0 the output is fully dry; at DryWet = 1 the output is fully wet.

The wet path contains a placeholder node (multiply by 1.0, passthrough). Replace this dummy with your effect processing, keeping `wet_gain` as the last node in the wet path. The dry path requires no modification.

## Signal Path

::signal-path
---
glossary:
  parameters:
    DryWet:
      desc: "Crossfade balance between dry and wet paths"
      range: "0.0 - 1.0"
      default: "0.0"
  functions:
    xfader:
      desc: "Linear crossfade controller: sends (1 - DryWet) to dry_gain and DryWet to wet_gain"
    sum outputs:
      desc: "Adds the dry and wet path outputs together"
---

```
// template.dry_wet - parallel dry/wet mixer
// audio in -> audio out

process(input) {
    dry = copy(input)
    wet = copy(input)

    // crossfade gains from xfader
    dry_level = 1.0 - DryWet
    wet_level = DryWet

    dry_path:
        dry = dry * dry_level

    wet_path:
        wet = user_processing(wet)      // replace dummy here
        wet = wet * wet_level

    output = sum outputs(dry, wet)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Mix
    params:
      - { name: DryWet, desc: "Crossfade balance. 0 = fully dry (unprocessed), 1 = fully wet (processed). The crossfade is linear.", range: "0.0 - 1.0", default: "0.0" }
---
::

The crossfade is linear, not equal-power. At DryWet = 0.5, both paths receive 0.5 gain (-6 dB each). For uncorrelated signals, expect approximately 3 dB less perceived loudness at the midpoint compared to the extremes. The xfader node sits inside the dry path but its switch targets cross the path boundary to reach wet_gain -- this is standard scriptnode modulation routing.

**See also:** $SN.container.split$ -- the underlying parallel container, $SN.control.xfader$ -- the crossfade controller
