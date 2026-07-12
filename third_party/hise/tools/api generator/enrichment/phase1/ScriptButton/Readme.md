# ScriptButton -- Class Analysis

## Brief
Toggle or momentary button component with filmstrip image support, radio group exclusion, and popup attachment.

## Purpose
ScriptButton is a binary on/off UI component that wraps a JUCE ToggleButton (via HiToggleButton). It supports toggle mode (click to switch), momentary mode (held-down only), radio group mutual exclusion, filmstrip-based image rendering with 2 or 6 frames, custom mouse cursors, and attachment of FloatingTile popup panels. Created via `Content.addButton()`, it inherits 34 methods from ScriptComponent and adds one of its own (`setPopupData`).

## Details

### Value Model

ScriptButton operates on a fixed binary range of 0 (off) and 1 (on). The base class `min` and `max` properties are deactivated -- the range cannot be changed. The `ValueToTextConverter` maps to `"Off"` and `"On"` for host automation display. Button values are integers, and `resetValueToDefault` casts the default through `(int)` to ensure proper comparison.

### Filmstrip Rendering

When a `filmstripImage` is set, the button uses `FilmstripLookAndFeel` to render from a sprite sheet. Two modes are supported:

| numStrips | Frame Layout |
|---|---|
| 2 | Frame 0 = off, Frame 1 = on |
| 6 | Frames 0-1 = normal off/on, Frames 2-3 = pressed off/on, Frames 4-5 = hover off/on |

If `isVertical` is true (default), frames are stacked top-to-bottom. If false, they are arranged left-to-right. The `scaleFactor` property scales the rendered output size. Only 2 and 6 are valid strip counts -- any other value falls back to the default look and feel. A custom LAF via `setLocalLookAndFeel` takes priority over filmstrip rendering.

### Radio Groups

Setting `radioGroup` to a non-zero integer groups buttons for mutual exclusion -- only one button in the group can be ON at any time. This uses JUCE's built-in radio group system. When a radio group button is exposed as a plugin parameter, it is automatically flagged as a meta parameter (because toggling one button affects others in the group).

### Momentary Mode

When `isMomentary` is true, the button turns ON on mouse-down and automatically returns to OFF on mouse-up. Right-clicks are ignored in both modes.

### setValueOnClick

When `setValueOnClick` is true, the button triggers its value change on mouse-down rather than the default mouse-up. This provides immediate response without waiting for the user to release the mouse button.

### Popup Attachment

Buttons can host a FloatingTile popup that toggles on click. See `setPopupData()` for the full configuration API including JSON structure and position parameters.

### Mouse Cursor

The `mouseCursor` property defaults to `"ParentCursor"`, which traverses up the parent component chain to inherit a cursor from a parent ScriptPanel (if one has a custom cursor set). Other values use standard JUCE cursor types. See the Constants table for all valid values.

### Colour Mapping

| Script Property | Visual Meaning |
|---|---|
| `bgColour` | Background / outline |
| `itemColour` | Fill top gradient |
| `itemColour2` | Fill bottom gradient |
| `textColour` | Button text |

### MIDI Learn

MIDI learn is enabled by default (`enableMidiLearn = true`). Right-clicking shows the MIDI learn popup. Note: setting `saveInPreset` to false also disables MIDI learn regardless of the `enableMidiLearn` setting.

## obtainedVia
`Content.addButton(name, x, y)`

## minimalObjectToken
btn

## Constants
None. ScriptButton has no `addConstant()` calls in its constructor.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `btn.set("numStrips", 4)` | `btn.set("numStrips", 2)` or `btn.set("numStrips", 6)` | Only 2-strip and 6-strip filmstrip modes are supported. Other values silently fall back to the default skin. |
| `btn.setPopupData(jsonData, [10, 20])` | `btn.setPopupData(jsonData, [10, 20, 300, 200])` | The position parameter must be a 4-element array [x, y, w, h] specifying offset and popup dimensions. Incomplete arrays throw a script error. |

## codeExample
```javascript
const var btn = Content.addButton("MyButton", 10, 10);
btn.set("text", "Enable");
```

## Alternatives
- `ScriptSlider` for continuous or stepped numeric value input instead of binary on/off.
- `ScriptPanel` for fully custom interactive areas with paint routines and mouse callbacks.
- `ScriptComboBox` for selecting from a list of named items instead of binary or radio-group selection.

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: ScriptButton is a simple binary component with no silent-failure preconditions or ordering dependencies that would benefit from parse-time diagnostics. Invalid filmstrip strip counts fall back gracefully, and invalid popup positions throw runtime errors.
