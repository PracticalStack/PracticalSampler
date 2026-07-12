# CSS Per-Component Mapping Survey

**Purpose:** Reference table mapping each UI component and FloatingTile content type to its CSS selectors, pseudo-states, pseudo-elements, and CSS variables. Assembled from old docs (phase3), `css.md` general reference, and snippet examples.

**Status:** Verified against old docs. Components marked with `[needs C++ verification]` have CSS support inferred but not confirmed in old docs.

---

## Plugin Components

### ScriptButton

| Aspect | Value |
|--------|-------|
| **HTML tag selector** | `button` |
| **Class selector** | `.scriptbutton` |
| **ID selector** | `#Button1` |
| **Pseudo-states** | `:hover`, `:active`, `:checked`, `:disabled` |
| **Pseudo-elements** | `::before`, `::after` |
| **CSS variables** | `--bgColour`, `--itemColour`, `--itemColour2`, `--textColour` (standard colour set) |
| **Notes** | `:checked` maps to button value being true. Straightforward 1:1 CSS mapping. |

**Source:** phase3/plugin-components/button.md

---

### ScriptSlider (Knob)

| Aspect | Value |
|--------|-------|
| **HTML tag selector** | *(none — no standard HTML equivalent)* |
| **Class selector** | `.scriptslider` |
| **ID selector** | `#Knob1` |
| **Pseudo-states** | `:hover`, `:active`, `:disabled` |
| **Pseudo-elements** | `::before`, `::after` |
| **CSS variables** | `--value` (normalized 0..1), `--bgColour`, `--itemColour`, `--itemColour2`, `--textColour` |
| **Additional sub-selectors** | `label` (value popup from `showValuePopup`), `input` (text input on shift-click) |
| **Notes** | Knob mode requires scripting to pass a path via `setStyleSheetProperty("valuePath", path, "path")` and use `var(--valuePath)` as `background-image`. Horizontal/vertical modes use `::before` for track, `::after` for thumb, with `var(--value)` for width/height calculation. `::selection` styles the text selection in the input box. |

**Source:** phase3/plugin-components/knob.md

---

### ScriptComboBox

| Aspect | Value |
|--------|-------|
| **HTML tag selector** | `select` |
| **Class selector** | `.scriptcombobox` |
| **ID selector** | `#ComboBox1` |
| **Pseudo-states** | `:hover`, `:active`, `:disabled` |
| **Pseudo-elements** | `::before`, `::after` (typically used for drop-down arrow) |
| **CSS variables** | Standard colour set |
| **Popup menu selectors** | `.popup` (menu background), `.popup-item` (individual items), `hr` (separator) |
| **Popup pseudo-states** | `.popup-item:hover`, `.popup-item:active` (selected), `.popup-item:disabled` |
| **Notes** | The popup menu styling also applies to right-click menus of other components that share the same CSS LAF object. The `useCustomPopup` property enables advanced popup syntax (`**Header**`, `~~disabled~~`, `SubMenu::`). |

**Source:** phase3/plugin-components/combobox.md

---

### ScriptLabel

| Aspect | Value |
|--------|-------|
| **HTML tag selector** | `label` |
| **Class selector** | `.scriptlabel` (implied by convention) |
| **ID selector** | `#Label1` |
| **Pseudo-states** | Standard set (`:hover`, `:active`, `:disabled`) |
| **Additional sub-selectors** | `input` (text editor while typing), `::selection` (text selection highlight) |
| **CSS variables** | `--bgColour`, `--textColour`, standard set |
| **Notes** | The `label` selector is also used globally for popup text overlays on Sliders, Tables, and SliderPacks. The `input` selector also styles text inputs on Slider shift-click. `caret-color` is supported for the text cursor. |

**Source:** phase3/plugin-components/label.md

---

### ScriptPanel

| Aspect | Value |
|--------|-------|
| **HTML tag selector** | `div` |
| **Class selector** | `.scriptpanel` |
| **ID selector** | `#Panel1` |
| **Pseudo-states** | `:hover`, `:active` (if `allowCallbacks` is set accordingly) |
| **Pseudo-elements** | `::before`, `::after` |
| **CSS variables** | `--bgColour`, `--itemColour`, `--itemColour2`, `--textColour`, plus custom variables via `setStyleSheetProperty()` |
| **Notes** | CSS styling is an alternative to using `setPaintRoutine()`. The `content` property can be set to `var(--titleText)` or similar custom variables for text rendering. Supports transitions for animations. |

**Source:** phase3/plugin-components/panel.md

---

### ScriptTable

| Aspect | Value |
|--------|-------|
| **HTML tag selector** | *(none — `table` is reserved for Viewport table mode)* |
| **Class selector** | `.scripttable` |
| **ID selector** | `#Table1` |
| **Pseudo-states** | `:hover`, `:active` (on table points) |
| **Pseudo-elements** | `::before` (used for rendering the table path) |
| **CSS variables** | `--tablePath` (Base64 path for the curve), `--playhead` (normalized x-position), standard colour set |
| **Sub-selectors** | `.tablepoint` (draggable points), `.tablemidpoint` (curve control mid-points), `.playhead` (position indicator), `label` (text overlay) |
| **Point pseudo-states** | `.tablepoint:hover`, `.tablepoint:active`, `.tablepoint:first-child`, `.tablepoint:last-child` |
| **Notes** | The playhead position is calculated with `calc(calc(var(--playhead) * 100%) - 1px)`. This is consistent with the AudioWaveform playhead rendering. |

**Source:** phase3/plugin-components/table.md, snippet: advanced-table-customization

---

### ScriptSliderPack

| Aspect | Value |
|--------|-------|
| **HTML tag selector** | *(none)* |
| **Class selector** | `.scriptsliderpack` |
| **ID selector** | `#SliderPack1` |
| **Pseudo-states** | `:hover`, `:active` (on individual sliders) |
| **Pseudo-elements** | `::before` (value rectangle), `::after` (step sequencer flash) |
| **CSS variables** | `--value` (normalized per-slider value), `--flash` (alpha for active step), `--linePath` (right-click line path), standard colour set |
| **Sub-selectors** | `.packslider` (individual sliders — styled like `.scriptslider` in horizontal mode), `.sliderpackline` (right-click line), `label` (text popup) |
| **Notes** | The `label` text-align/vertical-align controls the popup position within the slider pack, not text alignment within the label. The `.packslider::before` typically uses `height: calc(100% * var(--value))` with `bottom: 0%` for vertical fill. |

**Source:** phase3/plugin-components/sliderpack.md

---

### ScriptAudioWaveform

| Aspect | Value |
|--------|-------|
| **HTML tag selector** | *(none)* |
| **Class selector** | `.scriptaudiowaveform` |
| **ID selector** | `#AudioWaveform1` |
| **Pseudo-states** | `:disabled` (inactive zones) |
| **Pseudo-elements** | `::before` (waveform path rendering), `::after` (additional overlay) |
| **CSS variables** | `--waveformPath` (Base64 path), `--playhead` (normalized position), standard colour set |
| **Sub-selectors** | `.playhead` (playback position — consistent with Table), `.waveformedge` (draggable range edges), `label` (filename text) |
| **Edge pseudo-states** | `.waveformedge:hover`, `.waveformedge:active`, `.waveformedge:first-child` (left), `.waveformedge:last-child` (right) |
| **Notes** | The waveform path is drawn using `background-image: var(--waveformPath)`. The playhead uses the same `calc(100% * var(--playhead))` pattern as the Table. Inactive zones (outside the sample range) are rendered with the `:disabled` pseudo-state. |

**Source:** phase3/plugin-components/audio-waveform.md

---

### ScriptImage

| Aspect | Value |
|--------|-------|
| **HTML tag selector** | `img` (listed in css.md supported type ids) |
| **CSS support** | Minimal — the `img` type selector is listed in the CSS renderer but no old docs exist for ScriptImage CSS styling. |
| **Notes** | `[needs C++ verification]` — The `img` type ID is registered but it's unclear what CSS properties are meaningful for ScriptImage beyond basic positioning/sizing. |

---

### ScriptFloatingTile

| Aspect | Value |
|--------|-------|
| **HTML tag selector** | `div` (same as Panel) |
| **CSS support** | The FloatingTile wrapper itself uses `div`. Individual content types have their own CSS selectors (see Floating Tiles section below). |
| **Notes** | CSS applied to a FloatingTile targets the content type's rendering, not the wrapper. |

---

### ScriptedViewport

| Aspect | Value |
|--------|-------|
| **HTML tag selector** | *(none directly — uses class selector)* |
| **Class selector** | `.scriptviewport` |
| **ID selector** | `#Viewport1` |
| **Pseudo-states** | Standard set |
| **Sub-selectors (list mode)** | `tr` (list items), `scrollbar` (scrollbar thumb) |
| **List item pseudo-states** | `tr:hover`, `tr:active` (pressed), `tr:checked` (selected) |
| **Scrollbar pseudo-states** | `scrollbar:hover`, `scrollbar:active` |
| **Notes** | CSS is primarily useful in list mode (`useList = true`). The `scrollbar` width property overrides the `scrollbarThickness` component property. The `table` HTML tag is NOT used here — it's reserved in CSS for the HTML table element. |

**Source:** phase3/plugin-components/viewport.md

---

### ScriptDynamicContainer

| Aspect | Value |
|--------|-------|
| **CSS support** | The container itself has no visual representation. Child components created via `setData()` use the CSS selectors of their respective component types (e.g., a dynamic Button uses `button` / `.scriptbutton`). |
| **Notes** | No dedicated CSS selectors needed. |

---

## Floating Tile Content Types

### Keyboard

| Aspect | Value |
|--------|-------|
| **Class selectors** | `.keyboard` (background), `.whitekey` (white keys), `.blackkey` (black keys) |
| **Pseudo-states** | `:hover`, `:active` (key pressed) |
| **Pseudo-elements** | `::before`, `::after` (for multi-layer key rendering) |
| **CSS variables** | `--keyColour` (per-key colour from `Engine.setKeyColour()`), `--noteName` (C note labels, e.g. "C2") |
| **Setup requirements** | `CustomGraphics` = false, `DefaultAppearance` = false. Then set `KeyWidth` and `BlackKeyRatio` for proportions. |
| **Notes** | The CSS renderer adds a 10px margin to the black key area for shadow rendering. `:first-child` is available but noted as not fully working in old docs. |

**Source:** phase3/floating-tiles/keyboard.md

---

### PresetBrowser

| Aspect | Value |
|--------|-------|
| **Class selectors** | Multiple — see cheat sheet images in old docs |
| **Pseudo-states** | Standard set on sub-components |
| **Notes** | Can be fully styled with CSS. Sub-components (`button`, `label`) follow the same CSS rules as the corresponding plugin components. The context menu (from the "More" button) is styled like the ComboBox popup (`.popup`, `.popup-item`). A modal popup has its own `.modal` class selector where `padding` controls the modal background size. See `resources/images/extra/preset_css.png` and `preset_modal_css.png` for visual CSS selector maps. |

**Source:** phase3/floating-tiles/presetbrowser.md

---

### FilterDisplay

| Aspect | Value |
|--------|-------|
| **Class selector** | `.filtergraph` |
| **CSS variables** | `--filterPath` (frequency response path as `background-image`) |
| **Notes** | Basic CSS support for background + path rendering. |

**Source:** phase3/floating-tiles/filterdisplay.md

---

### DraggableFilterPanel

| Aspect | Value |
|--------|-------|
| **Class selectors** | `.filtergraph` (background + path, same as FilterDisplay), `.filterHandle` (drag handles) |
| **Handle pseudo-states** | `:hover`, `:active` (dragging), `:disabled` (inactive band), `:focus` (selected band) |
| **Handle CSS variables** | `--type` (filter type string), `--frequency` (prettified, e.g. "1.5kHz"), `--q` (resonance), `--gain` (gain in dB), `--index` (one-based band index) |
| **Notes** | When using `border` on the filter path, set `box-sizing: border-box` to avoid path scaling issues with margin/padding. |

**Source:** phase3/floating-tiles/draggablefilterpanel.md

---

### Other Floating Tiles

The following content types have **no documented CSS support** in the old docs. They may use the generic `div` selector inherited from the FloatingTile wrapper, but no component-specific CSS selectors are documented:

- AudioAnalyser
- AHDSRGraph / FlexAHDSRGraph
- Waveform / WavetableWaterfall
- Plotter
- MatrixPeakMeter
- ModulationMatrix / ModulationMatrixController
- CustomSettings
- PerformanceLabel
- ActivityLed
- TooltipPanel
- MidiOverlayPanel
- MidiSources / MidiChannelList
- MidiLearnPanel
- FrontendMacroPanel
- MPEPanel
- AboutPagePanel
- MarkdownPanel
- Empty

These content types are typically customised via LAF functions rather than CSS. `[needs C++ verification]` for any undocumented CSS support.

---

## Global / Shared CSS Selectors

These selectors apply across multiple components:

| Selector | Usage | Applies to |
|----------|-------|------------|
| `label` | Text popup/overlay | Slider value popup, Table text overlay, SliderPack text, AudioWaveform filename |
| `input` | Text input editor | Label text editing, Slider shift-click text input |
| `::selection` | Text selection highlight | Label, Slider text input |
| `.popup` | Context menu background | ComboBox dropdown, PresetBrowser "More" menu, Panel right-click |
| `.popup-item` | Context menu items | Same as above |
| `hr` | Menu separator | Popup menus using advanced syntax |
| `scrollbar` | Scrollbar thumb | Viewport, potentially other scrollable elements |
| `*` | Universal selector | All components in the same LAF scope (lowest priority) |

---

## CSS Variable Reference (Standard Colour Set)

All CSS-enabled components inherit these variables from their colour properties:

| Variable | Source property |
|----------|---------------|
| `--bgColour` | `bgColour` |
| `--itemColour` | `itemColour` |
| `--itemColour2` | `itemColour2` |
| `--textColour` | `textColour` |

Custom variables can be added via:
- `laf.setStyleSheetProperty(name, value, type)` — global to all components using that LAF
- `component.setStyleSheetProperty(name, value, type)` — per-component override

The `type` parameter supports: `""` (string), `"number"`, `"path"`, `"color"`.
