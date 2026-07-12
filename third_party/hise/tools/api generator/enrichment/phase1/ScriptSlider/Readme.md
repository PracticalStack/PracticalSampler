# ScriptSlider -- Class Analysis

## Brief
Single-value UI slider/knob with mode-aware ranges, styling, modifiers, and matrix target integration.

## Purpose
ScriptSlider is the Content-created component for editing one numeric value with slider or knob interaction. It extends ScriptComponent with mode-driven range formatting (frequency, decibel, time, tempo sync, pan, etc.), optional two-value range style helpers, popup text customization, and modifier-key action mapping. The class also bridges to matrix target IDs so a UI slider can visualize and forward matrix modulation behavior through dedicated connection helpers. It is created via Content and therefore follows the onInit component-creation lifecycle enforced by Content.

## Details

### Architecture Layers

1. Script API object: `ScriptingApi::Content::ScriptSlider` in `ScriptingApiContent.h/.cpp`
2. Runtime UI wrapper: `ScriptCreatedComponentWrappers::SliderWrapper` in `ScriptComponentWrappers.cpp`
3. Concrete JUCE widget: `HiSlider` in `MacroControlledComponents.h/.cpp`
4. Optional modulation bridge: `MatrixCableConnection` or `MultiMatrixModulatorConnection`

The script object owns properties and API methods. The wrapper translates properties into live `HiSlider` behavior.

### Mode and Range System

Mode conversion and range migration behavior are documented in `ScriptSlider.setMode`.
Normalized conversion and skew behavior are documented in `ScriptSlider.setValueNormalized` and `ScriptSlider.getValueNormalized`.
`middlePosition` now supports an explicit no-skew sentinel string `"disabled"`; legacy numeric values (including `-1`) are treated as numeric midpoint candidates and only affect skew when they are inside the current range.

### Style and Range-Only Helpers

Style selection is documented in `ScriptSlider.setStyle`.
Range-only helper behavior and preconditions are documented in `ScriptSlider.setMinValue`, `ScriptSlider.setMaxValue`, `ScriptSlider.getMinValue`, `ScriptSlider.getMaxValue`, and `ScriptSlider.contains`.

### Modifier Mapping

The modifiers object schema and action mapping are documented in `ScriptSlider.createModifiers` and `ScriptSlider.setModifiers`.

### Matrix Target Bridge

`matrixTargetId` property activates modulation display/forwarding infrastructure. ScriptSlider chooses a connection implementation based on MatrixIds target classification:

- modulator targets -> `MultiMatrixModulatorConnection`
- parameter/cable targets -> `MatrixCableConnection`

This path depends on GlobalModulatorContainer matrix data and GlobalRoutingManager cable updates.
Method-level connection setup and runtime refresh behavior are documented in `ScriptSlider.connectToModulatedParameter` and `ScriptSlider.updateValueFromProcessorConnection`.

## obtainedVia
`Content.addKnob("MySlider", x, y)`

## minimalObjectToken
sl

## Constants
None. ScriptSlider constructor has no active `addConstant()` registrations.

## Dynamic Constants
| Name | Type | Description |
|------|------|-------------|
| Modifiers.TextInput / FineTune / ResetToDefault / ContextMenu / ScaleModulation | String constants on script object | Action keys used by `setModifiers(action, modifiers)`. |
| Modifiers.disabled / noKeyModifier / shiftDown / rightClick / cmdDown / altDown / ctrlDown / doubleClick | Integer constants on script object | Modifier bitmasks and pseudo flags for modifier matching. |

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `sl.setMinValue(0.2);` while style is `Knob` | `sl.setStyle("Range"); sl.setMinValue(0.2);` | Range helper methods are only valid in `Range` style and otherwise do not apply as intended. |
| `sl.setMode("Db");` | `sl.setMode("Decibel");` | Invalid mode names are not accepted; use one of the exact mode strings from the options list. |
| `sl.setMidPoint(-1);` to disable skew in all ranges | `sl.setMidPoint("disabled");` | Numeric `-1` is a regular midpoint value now and may apply skew when the range includes `-1`. |
| `sl.setModifiers("Reset", [mods.altDown]);` | `sl.setModifiers("ResetToDefault", [mods.altDown]);` | Action key names are exact; unknown action keys do not map to runtime modifier actions. |

## codeExample
```javascript
const var sl = Content.addKnob("Drive", 20, 20);
sl.setMode("Decibel");
sl.setRange(-24.0, 12.0, 0.1);
```

## Alternatives
- `ScriptButton` - use for binary on/off interaction instead of numeric range control.
- `ScriptComboBox` - use for named discrete options instead of continuous numeric movement.
- `ScriptSliderPack` - use for editing many values at once instead of one value.

## Related Preprocessors
- `USE_BACKEND` -- enables illegal-range debug logging in normalized conversion methods.

## Diagrams

### scriptslider-matrix-target-chain
- **Brief:** Matrix Target Data Flow
- **Type:** topology
- **Description:** Shows how `matrixTargetId` on ScriptSlider resolves to target type via MatrixIds helpers, then instantiates either MultiMatrixModulatorConnection or MatrixCableConnection. Includes upstream providers (GlobalModulatorContainer matrix ValueTree and GlobalRoutingManager cables) and downstream callback back into the script processor control path.

## Diagnostic Ideas
Reviewed: Yes
Count: 2
- ScriptSlider.setMinValue / setMaxValue / getMinValue / getMaxValue / contains -- state precondition (style must be `Range`).
- ScriptSlider.setMode -- value validation (mode string should match known mode set).
