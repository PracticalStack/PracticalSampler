# Track A: Plugin Component Reference Pages

**Purpose:** Author reference pages for the 11 active HISE plugin components. This is a pure authoring task — all input data is already available. No C++ exploration is required.

**Input:** See per-component input manifest below.
**Output:** `ui_enrichment/pages/components/{ComponentName}.md`

---

## Process per component

For each component, follow these steps in order:

### Step 1: Gather inputs

Read the input files listed in the component's manifest (see "Input Manifests" section below). Do NOT skip any input — every file is there for a reason.

> **Property name casing:** Always use the exact property IDs from `ui_enrichment/resources/component_properties_reference.md`. Property names are case-sensitive and mostly camelCase. Do NOT use the C++ enum names (PascalCase) — these differ from the runtime property IDs that users pass to `set()` and `get()`. The reference file was generated from HISE runtime data and is the authoritative source.

### Step 2: Author the reference page

Write the MDC markdown file following the page template below. Work through each section:

1. **Frontmatter** — fill in all fields including `llmRef`
2. **Overview** — write 1-3 paragraphs from old docs + phase4b Readme. Focus on what the component IS and DOES, not how to script with it.
3. **Properties** — copy the property table from phase4b `set.md`. Add expanded descriptions for component-specific properties using old docs content. Note deactivated properties.
4. **LAF Customisation** (if applicable) — author a minimal LAF example using source material. Include the `obj` property table from `laf_style_guide.json`.
5. **CSS Styling** (if applicable) — document selectors, pseudo-states, variables from `css_component_mapping.md`. Include a CSS example from old docs (verified/updated).
6. **Notes** — any non-obvious behaviours, tips, gotchas from old docs or phase4b.
7. **See also** — placeholder for cross-reference post-processing.

### Step 3: Source completeness check

After writing the initial reference page, re-read all source material files and check for missing information:

- **Old docs (phase3)** — property descriptions, usage patterns, CSS examples, interaction quirks not yet on the page → add them. This is UI component documentation and belongs here.
- **Phase2 code examples** (`enrichment/phase2/{ClassName}/`) — these contain extracted code examples from real-world projects. Distill real-world usage patterns, gotchas, and non-obvious behaviours into the prose, property descriptions, or Notes section. Do not include verbatim — extract the insight.
- **MCP snippets** — if a snippet demonstrates a non-obvious component behaviour or interaction quirk, add the insight as a Note.
- **`custom_lookandfeel.md`** — check for LAF-related behavioural notes (e.g., LAF takes precedence over filmstrip rendering).
- **phase4b Readme** — check for scripting API insights that affect the UI page (e.g., precedence rules, property interaction effects).

**Skip** information that lives purely in the scripting API docs (method signatures, callback patterns, threading details) — the cross-reference audit will link to it.

Focus on: interaction quirks, precedence rules, practical code patterns for component-specific features (e.g., custom popup syntax for ComboBox), and gotchas from real-world usage.

### Step 3b: Forum community insights

Follow the three-phase workflow in `style-guide/forum-insights-guide.md`:

1. `forum-search.py search "{ComponentName}" --also "{natural name}" "{feature term}"`
2. Triage and fetch high-signal topics
3. `@extract-forum-insights` for structured insight extraction
4. `@verify-forum-claim` for bug/behaviour claims

**Target:** 3-5 new blocks per page (mix of warnings and tips).

### Step 4: Verify against gate checklist

Run through the gate checklist from `ui-component-enrichment.md`. Fix any gaps before moving on.

---

## Page Template (Track A)

```markdown
---
title: "{DisplayName}"
description: "{1-2 sentence description for SEO and page metadata}"
componentId: "{ScriptClassName}"
componentType: "plugin-component"
screenshot: "/images/v2/reference/ui-components/{lowercase-name}.png"
llmRef: |
  {ComponentName} (UI component)
  Create via: Content.addXxx("name", x, y)

  {1-2 sentence description.}

  Properties (component-specific):
    {property}: {brief description}
    ...

  Customisation:
    LAF: {function names or "none"}
    CSS: {selector} with {pseudo-states}
    Filmstrip: {yes/no}
seeAlso: []
commonMistakes:
  - title: "Descriptive title"
    wrong: "What users do incorrectly"
    right: "What they should do instead"
    explanation: "Why this matters"
---

![{DisplayName}](/images/v2/reference/ui-components/{lowercase-name}.png)

{Overview prose: 1-3 paragraphs. What this component is, when to use it,
key characteristics. Written for plugin developers. No C++ internals.
Link to scripting API class for method documentation.}

## Properties

{Brief intro sentence: "Set properties with `ComponentName.set(property, value)`."}

### Component-specific properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| *`propertyName`* | type | default | Description from phase4b set.md, expanded with old docs detail |

### Common properties

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `visible`, `enabled`, `locked` | Display and interaction state |
| ... | {copy from phase4b set.md} |

### Deactivated properties

{List any properties that are deactivated for this component, from phase4b set.md.}

## LAF Customisation

{Only include this section if the component has LAF functions.}

Register a custom look and feel to fully control the rendering of this component.

### LAF Functions

| Function | Description |
|----------|-------------|
| `drawFunctionName` | Brief description from laf_style_guide.json |

### `obj` Properties

{One table per LAF function, or a combined table if properties are similar.}

| Property | Type | Description |
|----------|------|-------------|
| `obj.area` | Array[x,y,w,h] | The component bounds |
| `obj.value` | double/bool | The current value |
| ... | ... | from laf_style_guide.json |

### Example

```javascript
// Minimal LAF example that replicates a recognisable version of the default appearance.
const var component = Content.addXxx("Component1", 10, 10);
const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawFunctionName", function(g, obj)
{
    // Draw background
    g.setColour(obj.bgColour);
    g.fillRoundedRectangle(obj.area, 3.0);

    // Draw value indicator
    // ... use obj.value, obj.hover, obj.down etc.
});

component.setLocalLookAndFeel(laf);
` ``

## CSS Styling

{Only include this section if the component has CSS support.}

{Brief intro: how CSS relates to this component.}

### Selectors

| Selector | Type | Description |
|----------|------|-------------|
| `{tag}` | HTML tag | Selects all {component} elements |
| `.{class}` | Class | Default class selector |
| `#{id}` | ID | Targets a specific component by name |

### Pseudo-states

| State | Description |
|-------|-------------|
| `:hover` | Mouse is over the component |
| `:active` | Mouse button is pressed |
| ... | from css_component_mapping.md |

### CSS Variables

| Variable | Description |
|----------|-------------|
| `--bgColour` | Background colour from component properties |
| ... | from css_component_mapping.md |

{Include any component-specific variables like `--value`, `--tablePath`, etc.}

### Sub-selectors

{Only if applicable — for components with additional CSS targets like `label`, `.playhead`, etc.}

### Example Stylesheet

```javascript
const var component = Content.addXxx("Component1", 10, 10);
const var laf = Content.createLocalLookAndFeel();

laf.setInlineStyleSheet("
{selector}
{
    background: #333;
    // ... from old docs, verified
}
");

component.setLocalLookAndFeel(laf);
` ``

## Notes

{Non-obvious behaviours, practical tips, gotchas. Use bullet points or short paragraphs.
Draw from old docs, phase4b common mistakes, and phase3 content.}

**See also:** {placeholder — populated during cross-reference post-processing}
```

---

## Input Manifests

### ScriptSlider

| Input | File |
|-------|------|
| Property table | `enrichment/phase4b/ScriptSlider/set.md` |
| Class overview | `enrichment/phase4b/ScriptSlider/Readme.md` |
| LAF obj properties | `laf_style_guide.json` → `ScriptComponents.ScriptSlider` |
| LAF examples | `custom_lookandfeel.md` § drawRotarySlider, § drawLinearSlider |
| LAF snippets | MCP: `big-small-knob-laf`, `horizontal-slider-laf`, `modulationdisplay`, `laf-tutorial-toolkit` |
| CSS reference | `css_component_mapping.md` § ScriptSlider |
| CSS example | `phase3/plugin-components/knob.md` § CSS Styling |
| Old docs | `phase3/plugin-components/knob.md` |
| Phase2 code examples | `enrichment/phase2/ScriptSlider/` (7 files: createModifiers, setModifiers, setValuePopupFunction, setLocalLookAndFeel, setValueNormalized, updateValueFromProcessorConnection) |
| Screenshot | `resources/images/plugin-components/knob.png` |

**Notes:** Two LAF functions (rotary + linear). Rich CSS with `--value` variable and scripted path for knob mode. Matrix modulation integration via `matrixTargetId` property. The modulation `obj` properties (`scaledValue`, `addValue`, `modulationActive`, etc.) should be documented in the LAF section.

---

### ScriptButton

| Input | File |
|-------|------|
| Property table | `enrichment/phase4b/ScriptButton/set.md` |
| Class overview | `enrichment/phase4b/ScriptButton/Readme.md` |
| LAF obj properties | `laf_style_guide.json` → `ScriptComponents.ScriptButton` |
| LAF examples | `custom_lookandfeel.md` § drawToggleButton |
| LAF snippets | MCP: `an-enhanced-button-laf`, `laf-tutorial-toolkit` |
| CSS reference | `css_component_mapping.md` § ScriptButton |
| CSS example | `phase3/plugin-components/button.md` § CSS Styling |
| Old docs | `phase3/plugin-components/button.md` |
| Phase2 code examples | — (no phase2 directory for ScriptButton) |
| Screenshot | `resources/images/plugin-components/button.png` |

**Notes:** Single LAF function. Straightforward CSS with `:checked` state. Document `radioGroup`, `isMomentary`, filmstrip properties.

---

### ScriptComboBox

| Input | File |
|-------|------|
| Property table | `enrichment/phase4b/ScriptComboBox/set.md` |
| Class overview | `enrichment/phase4b/ScriptComboBox/Readme.md` |
| LAF obj properties | `laf_style_guide.json` → `ScriptComponents.ScriptComboBox` |
| LAF examples | `custom_lookandfeel.md` § drawComboBox |
| LAF snippets | MCP: `laf-tutorial-toolkit` (inline combobox function) |
| CSS reference | `css_component_mapping.md` § ScriptComboBox |
| CSS example | `phase3/plugin-components/combobox.md` § CSS Styling |
| Old docs | `phase3/plugin-components/combobox.md` |
| Phase2 code examples | `enrichment/phase2/ScriptComboBox/` (8 files: addItem, changed, getItemText, set, setControlCallback, setLocalLookAndFeel, setValue) |
| Screenshot | `resources/images/plugin-components/combobox.png` |

**Notes:** Single LAF function. CSS includes popup menu styling (`.popup`, `.popup-item`, `hr`). Document `useCustomPopup` and the advanced popup syntax. The `items` property uses newline-separated values. Index starts at 1.

---

### ScriptLabel

| Input | File |
|-------|------|
| Property table | `enrichment/phase4b/ScriptLabel/set.md` |
| Class overview | `enrichment/phase4b/ScriptLabel/Readme.md` |
| LAF obj properties | — (no LAF functions) |
| CSS reference | `css_component_mapping.md` § ScriptLabel |
| CSS example | `phase3/plugin-components/label.md` § CSS Styling |
| Old docs | `phase3/plugin-components/label.md` |
| Phase2 code examples | `enrichment/phase2/ScriptLabel/` (3 files: changed, setControlCallback) |
| Screenshot | `resources/images/plugin-components/label.png` |

**Notes:** No LAF functions. CSS uses `label`, `input`, and `::selection` selectors. Document `editable`, `multiline`, font properties. The `label` selector is shared globally for popup text overlays.

---

### ScriptPanel

| Input | File |
|-------|------|
| Property table | `enrichment/phase4b/ScriptPanel/set.md` |
| Class overview | `enrichment/phase4b/ScriptPanel/Readme.md` |
| UI-specific content | `ui_enrichment/resources/scriptpanel_ui.md` |
| LAF obj properties | — (no LAF functions; uses paint routine instead) |
| CSS reference | `css_component_mapping.md` § ScriptPanel |
| CSS example | `phase3/plugin-components/panel.md` § CSS Styling |
| Old docs | `phase3/plugin-components/panel.md` |
| Phase2 code examples | `enrichment/phase2/ScriptPanel/` (9 files: loadImage, setFileDropCallback, setImage, setLoadingCallback, setMouseCallback, setPaintRoutine, setTimerCallback, startInternalDrag) |
| Screenshot | `resources/images/plugin-components/panel.png` |

**Notes:** Most complex component. No LAF functions — customised via paint routine (documented in scripting API). The UI page focuses on properties (especially `allowCallbacks` level table, `opaque`, popup properties with syntax), CSS styling, and a high-level description of the three callback systems (with links to the scripting API for details). Use `scriptpanel_ui.md` as the primary content source for panel-specific property documentation.

---

### ScriptTable

| Input | File |
|-------|------|
| Property table | `enrichment/phase4b/ScriptTable/set.md` |
| Class overview | `enrichment/phase4b/ScriptTable/Readme.md` |
| LAF obj properties | `laf_style_guide.json` → `ScriptComponents.ScriptTable` |
| LAF examples | `custom_lookandfeel.md` § Table Editor (5 functions) |
| LAF snippets | MCP: `advanced-table-customization` |
| CSS reference | `css_component_mapping.md` § ScriptTable |
| CSS example | `phase3/plugin-components/table.md` § CSS Styling |
| Old docs | `phase3/plugin-components/table.md` |
| Phase2 code examples | `enrichment/phase2/ScriptTable/` (5 files: getTableValue, registerAtParent, setLocalLookAndFeel, setTablePopupFunction) |
| Screenshot | `resources/images/plugin-components/table.png` |

**Notes:** Five LAF functions (background, path, point, midpoint, ruler). Rich CSS with `.tablepoint`, `.tablemidpoint`, `.playhead` sub-selectors and `--tablePath` / `--playhead` variables. The `advanced-table-customization` snippet shows both LAF and CSS side by side. Document `customColours` and `tableIndex` properties.

---

### ScriptSliderPack

| Input | File |
|-------|------|
| Property table | `enrichment/phase4b/ScriptSliderPack/set.md` |
| Class overview | `enrichment/phase4b/ScriptSliderPack/Readme.md` |
| LAF obj properties | `laf_style_guide.json` → `ScriptComponents.ScriptSliderPack` |
| LAF examples | `custom_lookandfeel.md` — (no SliderPack examples in old LAF docs) |
| LAF snippets | MCP: `shaded-sliderpack-laf` |
| CSS reference | `css_component_mapping.md` § ScriptSliderPack |
| CSS example | `phase3/plugin-components/sliderpack.md` § CSS Styling |
| Old docs | `phase3/plugin-components/sliderpack.md` |
| Phase2 code examples | `enrichment/phase2/ScriptSliderPack/` (7 files: referToData, setAllValueChangeCausesCallback, setControlCallback, setLocalLookAndFeel, setSliderAtIndex, setWidthArray) |
| Screenshot | `resources/images/plugin-components/sliderpack.png` |

**Notes:** Four LAF functions (background, flash overlay, right-click line, text popup). CSS has `.packslider` sub-selector styled like horizontal `.scriptslider`, with `--value` and `--flash` variables. The `shaded-sliderpack-laf` snippet is a good starting point for the LAF example.

---

### ScriptAudioWaveform

| Input | File |
|-------|------|
| Property table | `enrichment/phase4b/ScriptAudioWaveform/set.md` |
| Class overview | `enrichment/phase4b/ScriptAudioWaveform/Readme.md` |
| LAF obj properties | `laf_style_guide.json` → `ScriptComponents.ScriptAudioWaveform` |
| LAF examples | `custom_lookandfeel.md` § drawMidiDropper |
| LAF snippets | MCP: `zoomable-waveform-pos-zoom`, `zoomable-waveform-start-end` |
| CSS reference | `css_component_mapping.md` § ScriptAudioWaveform |
| CSS example | `phase3/plugin-components/audio-waveform.md` § CSS Styling |
| Old docs | `phase3/plugin-components/audio-waveform.md` |
| Phase2 code examples | `enrichment/phase2/ScriptAudioWaveform/` (4 files: getRangeEnd, setLocalLookAndFeel, setPlaybackPosition) |
| Screenshot | `resources/images/plugin-components/audio-waveform.png` |

**Notes:** Seven LAF functions (thumbnail background/text/path/range/ruler + render options + MIDI dropper). Rich CSS with `.waveformedge`, `.playhead` sub-selectors and `--waveformPath` / `--playhead` variables. The `getThumbnailRenderOptions` function is unusual — it returns configuration rather than drawing. Document `processorId` connection and display modes.

---

### ScriptImage

| Input | File |
|-------|------|
| Property table | `enrichment/phase4b/ScriptImage/set.md` |
| Class overview | `enrichment/phase4b/ScriptImage/Readme.md` |
| LAF obj properties | — (no LAF functions) |
| CSS reference | `css_component_mapping.md` § ScriptImage |
| Phase2 code examples | `enrichment/phase2/ScriptImage/` (2 files: setImageFile) |
| Old docs | — (no old docs page) |
| Screenshot | `resources/images/plugin-components/image.png` |

**Notes:** Simplest component. No LAF, minimal CSS (`img` tag). The phase4b `set.md` and `Readme.md` are the primary content sources. Document the `fileName`, `alpha`, and `allowCallbacks` properties.

---

### ScriptFloatingTile

| Input | File |
|-------|------|
| Property table | `enrichment/phase4b/ScriptFloatingTile/set.md` |
| Class overview | `enrichment/phase4b/ScriptFloatingTile/Readme.md` |
| LAF obj properties | — (no LAF functions on wrapper) |
| CSS reference | `css_component_mapping.md` § ScriptFloatingTile |
| Phase2 code examples | `enrichment/phase2/ScriptFloatingTile/` (3 files: setContentData, setLocalLookAndFeel) |
| Old docs | — (no dedicated old docs page) |
| Screenshot | `resources/images/plugin-components/floating-tile.png` |

**Notes:** This is the wrapper component. The page should document the wrapper properties (`ContentType`, `Data`, `Font`, `FontSize`, etc.) and include a summary table of all 26 content types with one-line descriptions and links to their Track B pages. No LAF or CSS of its own — those are on the individual content type pages.

**Content type summary table format:**

| ContentType | Description |
|------------|-------------|
| `PresetBrowser` | Preset browser for loading/saving user presets |
| `Keyboard` | Virtual MIDI keyboard |
| ... | ... |

---

### ScriptedViewport

| Input | File |
|-------|------|
| Property table | `enrichment/phase4b/ScriptedViewport/set.md` |
| Class overview | `enrichment/phase4b/ScriptedViewport/Readme.md` |
| LAF obj properties | — (no LAF functions) |
| CSS reference | `css_component_mapping.md` § ScriptedViewport |
| CSS example | `phase3/plugin-components/viewport.md` § CSS Styling |
| Old docs | `phase3/plugin-components/viewport.md` |
| Phase2 code examples | `enrichment/phase2/ScriptedViewport/` (9 files: getOriginalRowIndex, setControlCallback, setLocalLookAndFeel, setTableCallback, setTableColumns, setTableMode, setTableRowData, setValue) |
| Screenshot | `resources/images/plugin-components/viewport.png` |

**Notes:** No LAF functions. CSS is primarily useful in list mode (`useList = true`) with `tr` row items and `scrollbar`. Document the two modes (plain viewport vs list) and the `Items`, font, and scroll properties.

---

### ScriptDynamicContainer

| Input | File |
|-------|------|
| Property table | `enrichment/phase4b/ScriptDynamicContainer/set.md` |
| Class overview | `enrichment/phase4b/ScriptDynamicContainer/Readme.md` |
| User docs | `enrichment/phase4/auto/ScriptDynamicContainer/Readme.md` |
| setData docs | `enrichment/phase4b/ScriptDynamicContainer/setData.md` |
| LAF obj properties | — (no LAF functions) |
| CSS reference | — (no CSS; children use their own component CSS) |
| Phase2 code examples | — (no phase2 directory for ScriptDynamicContainer) |
| Old docs | — (no old docs) |
| Screenshot | — (no screenshot available) |

**Notes:** No LAF, no CSS, no screenshot. This is a container with no visual representation of its own. The page should focus on: what it does (dynamic component creation from JSON), supported child types, the data model, and how styling works (children inherit their parent component type's LAF/CSS). Use the phase4b and phase4 docs as primary sources. Include the `setData()` JSON format with supported type names.
