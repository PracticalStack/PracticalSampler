# ScriptLabel -- Class Analysis

## Brief
Editable text label component with font, alignment, and per-key update support.

## Purpose
ScriptLabel is a ScriptComponent subclass for displaying and editing text in the interface. It stores its value as a string in the `text` property and uses a dedicated wrapper that manages fonts, alignment, and editability. It participates in the Content lifecycle, so some settings are onInit-only. Preset save and restore use a dedicated `value` field to preserve text values.

## Details

### ScriptLabel-specific properties
| Property | Default | Notes |
|---|---|---|
| `fontName` | `"Arial"` | Special values: `"Default"`, `"Oxygen"`, `"Source Code Pro"` map to global fonts. Other strings resolve via the font registry or system font fallback. |
| `fontSize` | `13.0` | Slider range 1..200. |
| `fontStyle` | `"plain"` | Includes `"Password"` which switches to password character display. |
| `alignment` | `"centred"` | Uses justification names mapped by ApiHelpers. |
| `editable` | `true` | Controls clickability and editor opening. |
| `multiline` | `false` | Enables multi-line editing in the UI wrapper. |
| `updateEachKey` | `false` | When true, a timer dispatches control callbacks during editing. |

### Value semantics
- Use `ScriptLabel.getValue` to read the label text (mirrors the `text` property).
- Use `ScriptLabel.set` with `text` to update the label text; non-string values are ignored.
- `text` property writes are routed through the same string-only update path as `ScriptLabel.set`.

### Lifecycle constraint
- See `ScriptLabel.setEditable` (init-only; reports a script error if called after onInit).

### Preset integration
- `exportAsValueTree()` stores `value` from the current text.
- `restoreFromValueTree()` reads the `value` string and applies it to the text.

### Deactivated base properties
- `defaultValue`, `min`, `max`, `automationId` are deactivated for ScriptLabel.

## obtainedVia
`Content.addLabel(name, x, y)`

## minimalObjectToken
lb

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `lb.setEditable(false);` in `onControl` | `lb.setEditable(false);` in `onInit` | setEditable is onInit-only and reports a script error after initialization. |
| `lb.setValue(1);` | `lb.setValue("1");` | ScriptLabel only accepts strings for setValue; non-string values are ignored. |

## codeExample
```javascript
const var lb = Content.addLabel("Title", 10, 10);
lb.setEditable(true);
```

## Alternatives
- `ScriptPanel` -- draw custom text in a paint routine for full control.
- `ScriptComboBox` -- use a predefined list rather than free-form text input.

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: ScriptLabel methods are simple property setters and getters with direct runtime errors for lifecycle violations; there are no silent preconditions beyond the documented string-only value behavior.
