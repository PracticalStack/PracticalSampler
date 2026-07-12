---
title: "SliderPack"
description: "Multi-slider array editor for step sequencer lanes, multi-band parameters, and other array-valued controls."
componentId: "ScriptSliderPack"
componentType: "plugin-component"
screenshot: "/images/v2/reference/ui-components/sliderpack.png"
llmRef: |
  ScriptSliderPack (UI component)
  Create via: Content.addSliderPack("name", x, y)
  Scripting API: $API.ScriptSliderPack$

  Multi-slider array editor for SliderPackData values. Edits internal data, a processor external-data slot, or a referred SliderPackData source. Used for step-lane editing, shared multi-view data editing, and bulk value operations.

  Properties (component-specific):
    sliderAmount: number of sliders in the pack
    stepSize: value step resolution
    flashActive: flash the active slider step
    showValueOverlay: show value text overlay on drag
    SliderPackIndex: slider pack data slot index (PascalCase exception)
    mouseUpCallback: fire callback only on mouse release
    stepSequencerMode: step sequencer interaction mode

  Customisation:
    LAF: drawSliderPackBackground, drawSliderPackFlashOverlay, drawSliderPackRightClickLine, drawSliderPackTextPopup
    CSS: .scriptsliderpack with :hover, :active; .packslider sub-selector with --value, --flash variables; .sliderpackline, label
    Filmstrip: no
seeAlso: []
commonMistakes:
  - title: "Treating callback value as slider amplitude"
    wrong: "Using the value parameter directly as the slider's new value"
    right: "Use parseInt(value) as the edited index, then call getSliderValueAt(index) for the actual value"
    explanation: "The control callback's second argument is the index of the edited slider, not its value. Fetch the actual slider value with getSliderValueAt()."
  - title: "Importing many values with callbacks enabled"
    wrong: "Setting slider values in a loop without suppressing callbacks"
    right: "Call setAllValueChangeCausesCallback(false) during bulk import, then run one explicit downstream refresh"
    explanation: "Each slider value change fires a callback by default. Bulk imports cause callback storms that degrade performance. Suppress during import and trigger one update afterward."
  - title: "Mismatched setWidthArray length"
    wrong: "Calling setWidthArray with an array whose length differs from sliderAmount"
    right: "Ensure the width array length matches the current sliderAmount"
    explanation: "A length mismatch causes the layout to fall back to equal widths and breaks hit-testing assumptions."
  - title: "Confusing SliderPackIndex with parameterId"
    wrong: "Using parameterId to select a slider pack data slot"
    right: "Use SliderPackIndex to select the data slot, processorId to select the processor"
    explanation: "Slider pack binding uses the complex data path (processorId + SliderPackIndex), not the normal parameter path. parameterId targets processor parameters."
  - title: "Indexing into callback value as an array"
    wrong: "Console.print(value[0]) — treating the callback value as an array"
    right: "var index = parseInt(value); var amp = component.getSliderValueAt(index);"
    explanation: "The callback value is a single number (the edited slider's index), not an array of all slider values. Treating it as an array causes 'API call with undefined parameter' errors."
---

![SliderPack](/images/v2/reference/ui-components/sliderpack.png)

ScriptSliderPack is a multi-slider array editor that displays a row of vertical sliders for editing SliderPackData values. It is commonly used for step sequencer lanes, multi-band parameter editors, and any interface requiring an array of adjustable values.

The component can edit its own internal data, bind to a processor's external data slot via `processorId` and `SliderPackIndex`, or reference a shared `ScriptSliderPackData` source via `referToData()`. When `flashActive` is enabled, the currently active step flashes during playback — useful for step sequencer visualisation. The `stepSequencerMode` property changes interaction to click-to-set rather than click-and-drag.

## Properties

Set properties with `ScriptSliderPack.set(property, value)`.

### Component-specific properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| *`sliderAmount`* | int | `16` | Number of sliders displayed in the pack. Changing this updates the internal data size. Keep aligned with `setWidthArray()` if using custom widths. |
| *`stepSize`* | double | `0.01` | Value step resolution for each slider. Common values: `0.01` (smooth), `0.1` (coarse), `1.0` (integer steps). |
| *`flashActive`* | bool | `true` | When enabled, the currently active slider step flashes during playback. The flash intensity is available in LAF as `obj.intensity` and in CSS as `var(--flash)`. |
| *`showValueOverlay`* | bool | `true` | Show a text overlay displaying the current value and index when dragging a slider. Style the overlay with the `label` CSS selector or the `drawSliderPackTextPopup` LAF function. |
| *`SliderPackIndex`* | int | `0` | Selects which slider pack data slot on the connected processor to display. **Note:** This is a PascalCase exception — use `SliderPackIndex`, not `sliderPackIndex`. |
| *`mouseUpCallback`* | bool | `false` | When `true`, the control callback fires only on mouse release rather than during drag. Useful for step-lane editing where drag gestures should commit once. |
| *`stepSequencerMode`* | bool | `false` | Enables step sequencer interaction mode — clicking sets a slider directly to the clicked position rather than requiring drag. |

> [!Warning:sliderAmount is locked when bound to a processor] Once `processorId` connects the component to a processor's data slot, `sliderAmount` is controlled by the processor's data size and cannot be changed from the component side. Set `sliderAmount` before connecting, or manage the data size on the processor.

### Common properties

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `text`, `tooltip` | Display text and hover tooltip |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `min`, `max`, `defaultValue` | Value range and default for each slider |
| `bgColour`, `itemColour`, `itemColour2`, `textColour` | Colour properties |
| `parentComponent` | Parent component for layout nesting |
| `saveInPreset`, `useUndoManager`, `deferControlCallback` | Preset persistence, undo, and callback deferral |
| `processorId` | Connected processor module ID |

## LAF Customisation

Register a custom look and feel to control the rendering of the slider pack. Four functions cover the background, individual slider flash overlays, the right-click line, and the value text popup.

### LAF Functions

| Function | Description |
|----------|-------------|
| `drawSliderPackBackground` | Draws the background and all slider bars (called once, iterate using `numSliders` and `displayIndex`) |
| `drawSliderPackFlashOverlay` | Draws the flash overlay for the currently active step (called per slider when `flashActive` is enabled) |
| `drawSliderPackRightClickLine` | Draws the line when right-click dragging across sliders |
| `drawSliderPackTextPopup` | Draws the value text popup when dragging a slider |

### `obj` Properties (shared across all functions)

| Property | Type | Description |
|----------|------|-------------|
| `obj.id` | String | The slider pack's ID |
| `obj.bgColour` | int (ARGB) | Background colour |
| `obj.itemColour` | int (ARGB) | Thumb/slider colour |
| `obj.itemColour2` | int (ARGB) | Text box outline colour |
| `obj.textColour` | int (ARGB) | Track colour |

### Additional `obj` properties per function

#### `drawSliderPackBackground`

| Property | Type | Description |
|----------|------|-------------|
| `obj.numSliders` | int | Total number of sliders in the pack |
| `obj.displayIndex` | int | Index of the next slider to display |
| `obj.area` | Array[x,y,w,h] | The component bounds |

#### `drawSliderPackFlashOverlay`

| Property | Type | Description |
|----------|------|-------------|
| `obj.numSliders` | int | Total number of sliders |
| `obj.displayIndex` | int | Index of the slider being displayed |
| `obj.value` | double | The slider's current value |
| `obj.intensity` | double | Flash intensity (0.0–1.0, fades from 1.0 to 0.0) |
| `obj.area` | Array[x,y,w,h] | The individual slider bounds |

#### `drawSliderPackRightClickLine`

| Property | Type | Description |
|----------|------|-------------|
| `obj.x1` | double | Line start X position |
| `obj.y1` | double | Line start Y position |
| `obj.x2` | double | Line end X position |
| `obj.y2` | double | Line end Y position |

#### `drawSliderPackTextPopup`

| Property | Type | Description |
|----------|------|-------------|
| `obj.index` | int | Index of the currently dragged slider |
| `obj.value` | double | Value of the currently dragged slider |
| `obj.text` | String | The formatted text to display |
| `obj.area` | Array[x,y,w,h] | The component bounds |

### Example

```javascript
const var pack = Content.addSliderPack("SliderPack1", 10, 10);
pack.set("sliderAmount", 16);
pack.set("width", 300);
pack.set("height", 100);

const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawSliderPackBackground", function(g, obj)
{
    g.setColour(obj.bgColour);
    g.fillRoundedRectangle(obj.area, 3.0);

    // Draw each slider bar
    var sliderW = obj.area[2] / obj.numSliders;

    for (i = 0; i < obj.numSliders; i++)
    {
        var x = obj.area[0] + i * sliderW;
        g.setColour(0x22FFFFFF);
        g.fillRect([x + 1, obj.area[1], sliderW - 2, obj.area[3]]);
    }
});

laf.registerFunction("drawSliderPackFlashOverlay", function(g, obj)
{
    g.setColour(Colours.withAlpha(obj.itemColour, obj.intensity));
    g.fillRect(obj.area);
});

laf.registerFunction("drawSliderPackRightClickLine", function(g, obj)
{
    g.setColour(Colours.white);
    g.drawLine(obj.x1, obj.x2, obj.y1, obj.y2, 2.0);
});

laf.registerFunction("drawSliderPackTextPopup", function(g, obj)
{
    g.setColour(0xDD000000);
    var popupArea = [obj.area[0] + 5, obj.area[1] + 5, 60, 20];
    g.fillRoundedRectangle(popupArea, 3.0);
    g.setColour(Colours.white);
    g.setFont("Arial", 11.0);
    g.drawAlignedText(obj.text, popupArea, "centred");
});

pack.setLocalLookAndFeel(laf);
```

> [!Warning:Programmatic changes don't fire callbacks] `setSliderAtIndex()` and `setAllValues()` modify data silently — only user interaction triggers the control callback. If downstream logic depends on the callback, call `.changed()` explicitly after scripted changes.

## CSS Styling

CSS provides full control over the slider pack appearance. Individual sliders within the pack are styled using the `.packslider` class selector, which follows the same conventions as horizontal `.scriptslider` elements.

### Selectors

| Selector | Type | Description |
|----------|------|-------------|
| `.scriptsliderpack` | Class | Default class selector for the entire slider pack |
| `#SliderPack1` | ID | Targets a specific slider pack by component name |

### Pseudo-states

| State | Description |
|-------|-------------|
| `:hover` | Mouse is over the slider pack |
| `:active` | Mouse button is pressed |

### CSS Variables

| Variable | Description |
|----------|-------------|
| `--value` | Normalised value (0.0–1.0) per individual slider — use in `calc()` for height |
| `--flash` | Alpha value for the active step flash (0.0–1.0) |
| `--linePath` | Right-click line path as Base64 — use as `background-image` |
| `--bgColour` | Background colour from component properties |
| `--itemColour` | From the `itemColour` property |
| `--itemColour2` | From the `itemColour2` property |
| `--textColour` | Text colour from component properties |

### Sub-selectors

| Selector | Description |
|----------|-------------|
| `.packslider` | Individual sliders — styled like `.scriptslider` in horizontal mode |
| `.packslider::before` | Value rectangle (use `height: calc(100% * var(--value))` with `bottom: 0%`) |
| `.packslider::after` | Step sequencer flash indicator (use `var(--flash)` for colour) |
| `.sliderpackline` | Right-click drag line (use `background-image: var(--linePath)`) |
| `label` | Text popup overlay |

### Example Stylesheet

```javascript
const var sp = Content.addSliderPack("SliderPack1", 10, 10);
sp.set("itemColour", 0xFF5911A9);

const var laf = Content.createLocalLookAndFeel();

laf.setInlineStyleSheet("

.scriptsliderpack
{
	background-color: color-mix(in srgb, var(--itemColour) 50%, black);
	border-radius: 3px;
}

/** Style the individual sliders. */
.packslider
{
	margin: 0.5px;
	background-color: rgba(0, 0, 0, 0.1);
}

/** Value rectangle using the --value CSS variable. */
.packslider::before
{
	content: '';
	background-color: var(--itemColour);
	height: calc(100% * var(--value));
	width: 100%;
	border-radius: 2px;
	box-shadow: 0px 0px 3px black;
	bottom: 0%;
	margin-bottom: 9px;
}

.packslider::before:hover
{
	background-color: color-mix(in srgb, var(--itemColour) 90%, white);
	transition: background-color 0.2s;
}

.packslider::before:active
{
	background-color: color-mix(in srgb, var(--itemColour) 80%, white);
}

/** Step sequencer flash using the --flash CSS variable. */
.packslider::after
{
	content: '';
	height: 7px;
	background-color: color-mix(in rgba, white var(--flash), #444);
	border-radius: 1px;
	margin-bottom: 2px;
	box-shadow: 0px 0px 3px black;
	bottom: 0px;
}

/** Text popup overlay. */
label
{
	color: black;
	background-color: #ddd;
	margin: 3px;
	padding: 2px;
	text-align: right;
	vertical-align: top;
	border-radius: 3px;
	box-shadow: 0px 0px 4px black;
}

/** Right-click drag line. */
.sliderpackline
{
	background-image: var(--linePath);
	border: 2px solid white;
	box-shadow: 0px 0px 5px black;
	border-radius: 50%;
}
");

sp.setLocalLookAndFeel(laf);
```

> [!Tip:Use setControlCallback for per-slider routing] When mapping individual sliders to different processor parameters, use `setControlCallback()` with `parseInt(value)` as the index to route each slider to its target. This avoids duplicating parameter-setting code across multiple callbacks.

## Notes

- **Callback value is the edited index.** The control callback's second argument is the index of the edited slider, not the slider's value. Use `getSliderValueAt(parseInt(value))` to get the actual slider amplitude.
- **Complex data binding** uses `processorId` + `SliderPackIndex` (PascalCase). This is not the normal `processorId` + `parameterId` parameter path.
- **Shared data via `referToData()`.** Multiple slider packs can reference the same `ScriptSliderPackData` handle. Edits in one pack immediately appear in all views. Guard callback logic to prevent feedback loops when packs share data.

> [!Warning:Resize truncates data permanently] Reducing `sliderAmount` discards values beyond the new count. Increasing it back fills new slots with the default value — previous values are not restored. Cache data in a script array before resizing if you need to preserve it.

- **Suppress callbacks during bulk import.** Call `setAllValueChangeCausesCallback(false)` before loading patterns or bulk-setting values, then re-enable and trigger one downstream refresh. This prevents callback storms.
- **`mouseUpCallback = true`** is recommended for step-lane editing workflows where continuous drag callbacks are unnecessary and each gesture should commit once.
- **Custom slider widths** via `setWidthArray()` allow non-uniform slider sizes. The array length must match `sliderAmount` — mismatches cause fallback to equal widths.
- **Label position** in CSS: the `text-align` and `vertical-align` properties on the `label` selector control the popup's position within the slider pack area, not text alignment within the label itself.
- **Scalar helpers** like `getValueNormalized`, `setValueNormalized`, `setValueWithUndo`, and `updateValueFromProcessorConnection` are inherited but inactive on this component. Use the slider-pack-specific APIs (`getSliderValueAt`, `setSliderAtIndex`, `setAllValues`) instead.

**See also:** <!-- populated during cross-reference post-processing -->
