---
title: "Slider"
description: "Numeric control displayed as rotary knob, horizontal slider, vertical slider, or range slider with configurable range and display modes."
componentId: "ScriptSlider"
componentType: "plugin-component"
screenshot: "/images/v2/reference/ui-components/knob.png"
llmRef: |
  ScriptSlider (UI component)
  Create via: Content.addKnob("name", x, y)
  Scripting API: $API.ScriptSlider$

  Single-value slider or knob for editing numeric values. Supports mode-aware ranges and formatting (Frequency, Decibel, Time, TempoSync, Linear, Discrete, Pan, NormalizedPercentage), four visual styles (Knob, Horizontal, Vertical, Range), filmstrip rendering, matrix modulation integration, and custom value popups.

  Properties (component-specific):
    mode: value mode (Frequency, Decibel, Time, TempoSync, Linear, Discrete, Pan, NormalizedPercentage)
    style: visual style (Knob, Horizontal, Vertical, Range)
    stepSize: value step resolution
    middlePosition: midpoint for skewed display
    suffix: value suffix string
    filmstripImage: filmstrip image path
    numStrips: number of filmstrip frames
    isVertical: filmstrip orientation
    scaleFactor: filmstrip scale factor
    mouseSensitivity: mouse drag sensitivity
    dragDirection: drag direction (Diagonal, Vertical, Horizontal)
    showValuePopup: value popup display (No, Above, Below, Left, Right)
    showTextBox: show text box below slider
    scrollWheel: enable scroll wheel control
    enableMidiLearn: allow MIDI CC learn
    sendValueOnDrag: send value during drag vs on release
    matrixTargetId: matrix modulation target ID

  Customisation:
    LAF: drawRotarySlider (Knob style), drawLinearSlider (Horizontal, Vertical, Range styles)
    CSS: .scriptslider with :hover, :active, :disabled; --value variable; label, input sub-selectors
    Filmstrip: yes
seeAlso: []
commonMistakes:
  - title: "Calling setMinValue/setMaxValue outside Range style"
    wrong: "slider.setMinValue(0.5) when style is Knob, Horizontal, or Vertical"
    right: "Only use setMinValue/setMaxValue when style is set to Range"
    explanation: "Range helper methods only work with Range-style sliders. Calling them on other styles logs an error and has no effect."
  - title: "Using invalid mode strings"
    wrong: "slider.set(\"mode\", \"Db\") or slider.set(\"mode\", \"dB\")"
    right: "slider.set(\"mode\", \"Decibel\") — use exact mode names"
    explanation: "Mode names are case-sensitive. Invalid strings cause a silent fallback to the previous mode, leading to unexpected range and display behaviour."
  - title: "Stale UI after processor state restore"
    wrong: "Restoring module state without refreshing connected sliders"
    right: "Call updateValueFromProcessorConnection() on all connected sliders after state restore"
    explanation: "When processor parameters change outside slider interaction (preset load, undo), the slider UI shows stale values until the next manual drag. Explicitly pull the current value after restore."
  - title: "Expecting LAF to override filmstrip"
    wrong: "Setting both filmstripImage and a custom LAF, expecting LAF to render"
    right: "Clear filmstripImage (set to 'Use default skin') before applying a custom LAF"
    explanation: "When a filmstrip image is set, it takes precedence over LAF rendering. The LAF draw function is not called."
  - title: "Custom LAF hides the knob name and value text"
    wrong: "Registering drawRotarySlider and expecting the name and value to appear automatically"
    right: "Draw text manually in the LAF callback using g.drawAlignedText(obj.text, ...) and g.drawAlignedText(obj.valueAsText, ...)"
    explanation: "A custom LAF takes over all rendering. The default name label and value display are part of the default renderer — once you register a custom draw function, you are responsible for drawing everything including text. Use obj.text for the component name and obj.valueAsText for the formatted value."
---

![Slider](/images/v2/reference/ui-components/knob.png)

ScriptSlider is a single-value numeric control that can appear as a rotary knob, horizontal slider, vertical slider, or dual-handle range slider. It is the primary component for editing parameter values in HISE plugin interfaces.

The slider supports mode-aware value ranges and formatting — setting the `mode` property to `Frequency`, `Decibel`, `Time`, or other modes automatically configures sensible min/max/step/suffix defaults and display formatting. The `style` property controls the visual presentation: `Knob` renders a rotary control, `Horizontal` and `Vertical` render linear sliders, and `Range` renders a dual-handle slider for selecting a value range.

When connected to a Matrix Modulator via `matrixTargetId` (or by binding to a modulator's Value parameter), the slider provides real-time modulation visualisation through additional LAF `obj` properties (`scaledValue`, `addValue`, `modMinValue`, `modMaxValue`, etc.) and built-in interaction features for editing modulation connections.

## Properties

Set properties with `ScriptSlider.set(property, value)`.

### Component-specific properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| *`mode`* | String | `"Linear"` | Value mode controlling range defaults and display formatting. Options: `Frequency`, `Decibel`, `Time`, `TempoSync`, `Linear`, `Discrete`, `Pan`, `NormalizedPercentage`. Switching modes can auto-migrate range settings if the old defaults were untouched. |
| *`style`* | String | `"Knob"` | Visual style. `Knob` renders a rotary control, `Horizontal` and `Vertical` render linear sliders, `Range` renders a dual-handle range slider. Determines which LAF function is called (`drawRotarySlider` vs `drawLinearSlider`). |
| *`stepSize`* | double | `0.01` | Value step resolution. Common values: `0.01` (smooth), `0.1` (coarse), `1.0` (integer steps). |
| *`middlePosition`* | double | `-1` | Sets the midpoint for skewed (non-linear) display. When set to a value within the slider range, the display curve skews so that the midpoint sits at the centre of travel. Set to `-1` to disable skewing. |
| *`suffix`* | String | `""` | Suffix appended to the value display (e.g. `" Hz"`, `" dB"`, `" ms"`). Available in LAF as `obj.suffix` and `obj.valueSuffixString`. |
| *`filmstripImage`* | String | `"Use default skin"` | Path to a filmstrip image for rendering. When set, the filmstrip takes precedence over LAF rendering. Set to `"Use default skin"` to use the default renderer or a custom LAF. |
| *`numStrips`* | int | `0` | Number of frames in the filmstrip image. Must match the actual frame count — a mismatch causes rendering glitches or a blank knob. |
| *`isVertical`* | bool | `true` | Whether the filmstrip is arranged vertically (frames stacked top to bottom). |
| *`scaleFactor`* | double | `1` | Scale factor for the filmstrip image. Use `2` for retina/HiDPI filmstrips. |
| *`mouseSensitivity`* | double | `1` | Mouse drag sensitivity multiplier. Higher values = faster response to dragging. |
| *`dragDirection`* | String | `"Diagonal"` | Drag direction for value changes: `Diagonal`, `Vertical`, `Horizontal`. |
| *`showValuePopup`* | String | `"No"` | Show a floating value popup during drag: `No`, `Above`, `Below`, `Left`, `Right`. Style the popup with the `label` CSS selector or `setValuePopupFunction()` for custom formatting. |
| *`showTextBox`* | bool | `false` | Show a text box below the slider displaying the current value. Only available in Horizontal and Vertical styles. |
| *`scrollWheel`* | bool | `true` | Enable scroll wheel control for value changes. |
| *`enableMidiLearn`* | bool | `true` | Allow MIDI CC learn via right-click context menu. |
| *`sendValueOnDrag`* | bool | `true` | When `true`, sends value updates continuously during drag. When `false`, sends only on mouse release. |
| *`matrixTargetId`* | String | `""` | Registers this slider as a modulation target for the matrix modulation system. If the string matches a target ID of a Matrix Modulator, connects to that module. Otherwise creates a custom modulation slot. Enables modulation visualisation and interaction features in the LAF callback. |

### Common properties

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `text`, `tooltip` | Display text and hover tooltip |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `min`, `max`, `defaultValue` | Value range and default |
| `bgColour`, `itemColour`, `itemColour2`, `textColour` | Colour properties |
| `parentComponent` | Parent component for layout nesting |
| `saveInPreset`, `useUndoManager`, `deferControlCallback` | Preset persistence, undo, and callback deferral |
| `isPluginParameter`, `pluginParameterName`, `pluginParameterGroup`, `automationID` | DAW automation |
| `macroControl`, `isMetaParameter`, `linkedTo` | Macro control and parameter linking |
| `processorId`, `parameterId` | Module parameter connection |

## LAF Customisation

Register a custom look and feel to fully control the rendering of this component. The `style` property determines which function is called: `drawRotarySlider` for Knob style, `drawLinearSlider` for Horizontal, Vertical, and Range styles.

### LAF Functions

| Function | Description |
|----------|-------------|
| `drawRotarySlider` | Draws the rotary (knob) style slider |
| `drawLinearSlider` | Draws horizontal, vertical, and range style sliders |

### `obj` Properties (shared)

These properties are available in both `drawRotarySlider` and `drawLinearSlider`:

| Property | Type | Description |
|----------|------|-------------|
| `obj.id` | String | The component's ID |
| `obj.enabled` | bool | Whether the slider is enabled |
| `obj.text` | String | The slider's text property |
| `obj.area` | Array[x,y,w,h] | The component bounds |
| `obj.value` | double | The current raw value |
| `obj.valueNormalized` | double | Value normalised to 0.0–1.0 |
| `obj.valueAsText` | String | The value formatted as display text |
| `obj.valueSuffixString` | String | The value with suffix formatted as text |
| `obj.suffix` | String | The suffix string (e.g. `"Hz"`, `"dB"`) |
| `obj.skew` | double | The skew factor for non-linear scaling |
| `obj.min` | double | The minimum slider value |
| `obj.max` | double | The maximum slider value |
| `obj.clicked` | bool | Whether the mouse button is down |
| `obj.hover` | bool | Whether the mouse is over the slider |
| `obj.bgColour` | int (ARGB) | Background colour |
| `obj.itemColour1` | int (ARGB) | First item colour |
| `obj.itemColour2` | int (ARGB) | Second item colour |
| `obj.textColour` | int (ARGB) | Text colour |
| `obj.parentType` | String | ContentType of parent FloatingTile (if any) |

#### Modulation properties (shared)

When the slider is connected to a modulation target (via `matrixTargetId` or a Matrix Modulator's Value parameter), these additional properties are available in both functions:

| Property | Type | Description |
|----------|------|-------------|
| `obj.scaledValue` | double | Normalised value after all "Scale" mode modulation connections |
| `obj.addValue` | double | Normalised value after all unipolar/bipolar modulation offsets |
| `obj.modulationActive` | bool | Whether any modulation connection is active |
| `obj.modMinValue` | double | Minimum of the modulated range (0.0–1.0) |
| `obj.modMaxValue` | double | Maximum of the modulated range (0.0–1.0) |
| `obj.lastModValue` | double | Smoothed last modulation value |

To calculate the actual modulated display value, combine: `Math.range(obj.scaledValue + obj.addValue, 0.0, 1.0)`.

### Additional `obj` properties per function

`drawRotarySlider` uses only the shared properties above.

#### `drawLinearSlider`

| Property | Type | Description |
|----------|------|-------------|
| `obj.style` | int | Slider style code: Horizontal = 2, Vertical = 3, Range = 9 |
| `obj.valueRangeStyleMin` | double | Minimum value for Range-style sliders |
| `obj.valueRangeStyleMax` | double | Maximum value for Range-style sliders |
| `obj.valueRangeStyleMinNormalized` | double | Normalised min value for Range-style sliders |
| `obj.valueRangeStyleMaxNormalized` | double | Normalised max value for Range-style sliders |

### Example

```javascript
// Minimal rotary knob with modulation ring display.
const var knob = Content.addKnob("Knob1", 10, 10);
knob.set("style", "Knob");
knob.set("width", 80);
knob.set("height", 80);

const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawRotarySlider", function(g, obj)
{
    var a = obj.area;
    var size = Math.min(a[2], a[3]);
    var knobArea = [a[0] + (a[2] - size) / 2 + 6,
                    a[1] + 6, size - 12, size - 12];

    // Draw track arc
    var arc = Content.createPath();
    arc.addArc([0.0, 0.0, 1.0, 1.0], -2.4, 2.4);
    g.setColour(0x33FFFFFF);
    g.drawPath(arc, knobArea, 4.0);

    // Draw value arc
    var valueArc = Content.createPath();
    valueArc.addArc([0.0, 0.0, 1.0, 1.0],
                    -2.4, -2.4 + obj.valueNormalized * 4.8);
    g.setColour(obj.itemColour1);
    g.drawPath(valueArc, knobArea, 4.0);

    // Draw modulation range (if active)
    if (obj.modulationActive)
    {
        var modArc = Content.createPath();
        modArc.addArc([0.0, 0.0, 1.0, 1.0],
            -2.4 + obj.modMinValue * 4.8,
            -2.4 + obj.modMaxValue * 4.8);
        g.setColour(0x33FFFFFF);
        g.drawPath(modArc, knobArea, 3.0);
    }

    // Draw centre knob
    g.setColour(obj.bgColour);
    var inner = [knobArea[0] + 8, knobArea[1] + 8,
                 knobArea[2] - 16, knobArea[3] - 16];
    g.fillEllipse(inner);

    // Draw value text
    g.setColour(obj.textColour);
    g.setFont("Arial", 11.0);
    g.drawAlignedText(obj.valueAsText, inner, "centred");
});

laf.registerFunction("drawLinearSlider", function(g, obj)
{
    var a = obj.area;

    // Draw track
    g.setColour(0x33FFFFFF);
    g.fillRoundedRectangle(a, 3.0);

    // Draw value fill
    g.setColour(obj.itemColour1);

    if (obj.style == 2) // Horizontal
    {
        var w = a[2] * obj.valueNormalized;
        g.fillRoundedRectangle([a[0], a[1], w, a[3]], 3.0);
    }
    else if (obj.style == 3) // Vertical
    {
        var h = a[3] * obj.valueNormalized;
        g.fillRoundedRectangle([a[0], a[1] + a[3] - h, a[2], h], 3.0);
    }
    else if (obj.style == 9) // Range
    {
        var x1 = a[2] * obj.valueRangeStyleMinNormalized;
        var x2 = a[2] * obj.valueRangeStyleMaxNormalized;
        g.fillRoundedRectangle([a[0] + x1, a[1], x2 - x1, a[3]], 3.0);
    }
});

knob.setLocalLookAndFeel(laf);
```

> [!Tip:Draw name and value text yourself in custom LAF] A custom LAF takes over all rendering — the default name label and value display disappear. Use `obj.text` for the component name (e.g. below the knob) and `obj.valueAsText` or `obj.valueSuffixString` for the formatted value (e.g. inside the knob). This is the most common surprise for users switching from filmstrip to LAF rendering.

## CSS Styling

CSS provides control over slider and knob appearance using class selectors and CSS variables. Knob mode requires a scripted path passed via `setStyleSheetProperty()` for the value arc, while linear modes use `::before` and `::after` pseudo-elements with the `--value` variable.

### Selectors

| Selector | Type | Description |
|----------|------|-------------|
| `.scriptslider` | Class | Default class selector for all sliders/knobs |
| `#Knob1` | ID | Targets a specific slider by component name |

### Pseudo-states

| State | Description |
|-------|-------------|
| `:hover` | Mouse is over the slider |
| `:active` | Mouse button is pressed (dragging) |
| `:disabled` | Slider is disabled |

### Pseudo-elements

| Element | Description |
|---------|-------------|
| `::before` | Track (horizontal/vertical) or full arc (knob) |
| `::after` | Thumb (horizontal/vertical) or value arc (knob) |

### CSS Variables

| Variable | Description |
|----------|-------------|
| `--value` | Normalised slider value (0.0–1.0) — use in `calc()` for width/height |
| `--bgColour` | Background colour from component properties |
| `--itemColour` | From the `itemColour` property |
| `--itemColour2` | From the `itemColour2` property |
| `--textColour` | Text colour from component properties |

### Sub-selectors

| Selector | Description |
|----------|-------------|
| `label` | Value popup (enabled via `showValuePopup` property) |
| `input` | Text input box (shown on shift-click) |
| `::selection` | Text selection highlight in the input box |

### Example Stylesheet

```javascript
const var s = Content.addKnob("Knob1", 10, 10);
s.set("showTextBox", false);
s.set("style", "Horizontal");
s.set("height", 24);

const var laf = Content.createLocalLookAndFeel();

laf.setInlineStyleSheet("
.scriptslider
{
	background: #333;
	margin: 0px;
	border: 2px solid #555;
	border-radius: 5px;
	box-shadow: inset 0px 2px 5px black;
}

/** Value rectangle using the --value CSS variable. */
.scriptslider::before
{
	content: '';
	background: #555;
	margin: 4px;
	width: max(10px, calc(5px + calc(100% * var(--value))));
	border-radius: 3px;
}

.scriptslider::before:hover { background: #666; }
.scriptslider::before:active { background: linear-gradient(to bottom, #777, #666); }
");

s.setLocalLookAndFeel(laf);
```

For knob-style CSS, create a path in script using `Path.addArc()`, convert to a stroked path, and pass it to the component with `component.setStyleSheetProperty("valuePath", strokedPath, "path")`. Then reference it in CSS as `background-image: var(--valuePath)`. Use a Broadcaster attached to the component value to update the path when the value changes.

> [!Warning:Linear sliders jump to the click position] Horizontal and Vertical style sliders set the value to the mouse position on click, rather than dragging relative to the current value. This is the default JUCE slider behaviour. To get knob-like relative drag on a linear slider, set `dragDirection` to `Vertical` or `Diagonal` — the value then changes relative to the initial click position rather than jumping.

## Notes

- **Four style modes** determine both rendering and interaction: `Knob` uses rotary drag and calls `drawRotarySlider`; `Horizontal`, `Vertical`, and `Range` use linear drag and call `drawLinearSlider`. Switch styles at runtime with `set("style", "Horizontal")`.
- **Mode sets sensible defaults.** Switching `mode` to `Frequency`, `Decibel`, etc. auto-configures `min`, `max`, `stepSize`, `suffix`, and `middlePosition` — but only if the previous defaults were untouched. After manual range changes, mode switching preserves your custom values.
- **Filmstrip takes precedence over LAF.** If `filmstripImage` is set to anything other than `"Use default skin"`, the filmstrip renderer is used and the LAF draw function is not called. Clear the filmstrip property before applying a custom LAF.
- **Value popup formatting** can be customised with `setValuePopupFunction(callback)` — the callback receives the raw value and returns a display string. Share one formatter function across multiple sliders for consistency.

> [!Tip:Map numeric values to custom display strings] For sliders that select from a discrete set (e.g., note names, waveform types), use `setValuePopupFunction()` with a lookup array: `slider.setValuePopupFunction(function(v){ return ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"][v]; });`. This replaces the numeric popup with meaningful labels without needing a ComboBox.

- **Modifier mappings** let you reassign gestures like text input, fine-tune, and reset-to-default. Call `createModifiers()` once to get the constant set, then apply with `setModifiers()`. Reuse one modifier schema across slider collections for consistent interaction.
- **After processor state restore,** call `updateValueFromProcessorConnection()` on all connected sliders to pull the current parameter value from the processor. Without this, sliders show stale values until the next manual interaction.
- **Range-style sliders** expose `setMinValue()` and `setMaxValue()` for setting the two handles. These methods are only valid when `style` is `Range` — calling them on other styles logs an error.

**See also:** <!-- populated during cross-reference post-processing -->
