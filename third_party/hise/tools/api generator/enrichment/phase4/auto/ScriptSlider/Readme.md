# ScriptSlider

`ScriptSlider` is the standard single-value UI control for knobs and sliders in HISEScript, with built-in value formatting, gesture mapping, and parameter/matrix integration.

It is most useful when you need one control type to cover both straightforward parameter editing and deeper workflows such as modulation display, custom popup formatting, and shared gesture behaviour across many controls. Create it in `onInit` using `Content.addKnob(...)`, then configure mode, range, and interaction rules before runtime callbacks begin.

```javascript
const var sl = Content.addKnob("Drive", 20, 20);
sl.setMode("Decibel");
sl.setRange(-24.0, 12.0, 0.1);
```

## Mode Guide

`setMode(...)` is the most important behavioural switch on ScriptSlider because it affects both conversion and display formatting.

| Mode | Typical range profile | What users see | Best for |
| -- | -- | -- | -- |
| `Frequency` | Frequency-oriented defaults | Hz / kHz style display | Cutoff, oscillator frequency, tuning controls |
| `Decibel` | dB-oriented defaults | dB values | Gain, drive, threshold, output level |
| `Time` | Time-oriented defaults | Time units | Attack/release, delay time, envelope timings |
| `TempoSync` | Indexed tempo-division range | Musical division labels | Synced LFO rates, delay sync, sequencer timing |
| `Linear` | Plain numeric range | Raw numeric values + suffix | Generic controls with direct linear scaling |
| `Discrete` | Step-wise mapping | Integer-like steps | Selector-style stepped controls |
| `Pan` | Centered bipolar profile | Left/Right pan labeling | Stereo balance and pan controls |
| `NormalizedPercentage` | 0..1 profile | Percentage representation | Mix, macro, modulation amount/intensity |

If the previous mode still uses untouched defaults, switching mode migrates default range settings (min/max/step/suffix/midpoint) to the new mode profile.

Modifier actions exposed by `createModifiers()` and consumed by `setModifiers()` are:

| Name | Default gesture | Typical use |
| -- | -- | -- |
| `ScriptSlider.Modifiers.TextInput` | Shift + click | Type exact values |
| `ScriptSlider.Modifiers.ResetToDefault` | Double-click | Return to default quickly |
| `ScriptSlider.Modifiers.ContextMenu` | Right-click | MIDI learn and related actions |
| `ScriptSlider.Modifiers.FineTune` | Cmd/Ctrl/Alt drag | Smaller edit increments |
| `ScriptSlider.Modifiers.ScaleModulation` | Host-dependent | Modulation amount scaling |

When combining modifier flags, use bitwise OR for alternatives and an array for combined requirements:

- `mods.rightClick | mods.altDown` means either gesture can trigger the action.
- `[mods.doubleClick, mods.shiftDown]` means both must be true.
- `[mods.doubleClick, mods.noKeyModifier]` disambiguates plain double-click from modified double-click mappings.

The matrix target bridge is driven by the `matrixTargetId` property and connection helpers.

> [!Tip:Set modifier mappings once in onInit] Modifier mappings are intended to be set once in `onInit`. They persist across recompiles and only reset when the UI is rebuilt.

## Common Mistakes

- **Range style required for min/max values**
  **Wrong:** `sl.setMinValue(0.2);` while style is `Knob`
  **Right:** `sl.setStyle("Range"); sl.setMinValue(0.2);`
  *Range helper methods only apply in `Range` style, so call them after switching style.*

- **Use Decibel not Db for mode name**
  **Wrong:** `sl.setMode("Db");`
  **Right:** `sl.setMode("Decibel");`
  *Mode names must match the supported strings exactly.*

- **Pass JSON config not separate arguments**
  **Wrong:** `sl.setModifiers("Reset", [mods.altDown]);`
  **Right:** `sl.setModifiers("ResetToDefault", [mods.altDown]);`
  *Action keys are exact identifiers. Unknown keys are ignored by the runtime mapping logic.*

- **Share modifier config across sliders**
  **Wrong:** Recreating modifier objects for every slider in a loop
  **Right:** Create one modifiers object, then reuse it across the slider group
  *Shared mappings keep interaction behaviour consistent and make remapping easier later.*

- **setValueNormalized does not trigger callback**
  **Wrong:** Calling `setValueNormalized()` and assuming callback-driven logic already ran
  **Right:** Call `setValueNormalized(...)`, then `changed()` when you need callback side effects
  *Value assignment updates state, but callback-based workflows still need an explicit trigger.*

- **Use disabled string to remove skew**
  **Wrong:** `sl.setMidPoint(-1);` to disable skew in every range
  **Right:** `sl.setMidPoint("disabled");`
  *Numeric `-1` is now treated like any other midpoint value and can apply skew when the range includes `-1`.*
