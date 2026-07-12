# Track B: Floating Tile Reference Pages

**Purpose:** Author reference pages for the 26 HISE floating tile content types. Unlike Track A, this track includes one C++ exploration step per content type to extract JSON configuration properties.

**Input:** C++ panel classes + existing resources (see below).
**Output:** `ui_enrichment/pages/floating-tiles/{ContentType}.md`

---

## Process per content type

### Step 1: Extract JSON configuration properties from C++

Each floating tile content type is implemented as a panel class that registers its own properties. Extract the property list, types, and defaults from C++.

**Where to look:**

1. Find the panel class for the content type. The class name → ContentType mapping is in `doc_builders/laf-extraction.md` § Step 3a.
2. Look for `getPropertyList()` or the property enum in the panel class header.
3. Look for `getDefaultProperty()` for default values.
4. Cross-reference against the old docs frontmatter (`phase3/floating-tiles/{name}.md` `properties:` YAML field) to verify completeness.

**Common panel class locations:**

| Content Type | Class | File |
|-------------|-------|------|
| PresetBrowser | `PresetBrowserPanel` | `hi_core/hi_components/floating_layout/FrontendPanelTypes.h` |
| Keyboard | `MidiKeyboardPanel` | `hi_core/hi_components/floating_layout/FrontendPanelTypes.h` |
| AHDSRGraph | `AhdsrEnvelope::Panel` | `hi_core/hi_modules/modulators/mods/AhdsrEnvelope.h` |
| AudioAnalyser | `AudioAnalyserComponent::Panel` | `hi_core/hi_components/floating_layout/FrontendPanelTypes.h` |
| FilterDisplay | `FilterGraph::Panel` | `hi_core/hi_components/floating_layout/FrontendPanelTypes.h` |
| DraggableFilterPanel | `FilterDragOverlay::Panel` | `hi_core/hi_components/floating_layout/FrontendPanelTypes.h` |
| Waveform | `WaveformComponent::Panel` | `hi_core/hi_components/floating_layout/FrontendPanelTypes.h` |
| MatrixPeakMeter | `MatrixPeakMeter` | `hi_core/hi_components/floating_layout/FrontendPanelTypes.h` |

For other panel classes, search for `SET_PANEL_NAME` in header files, or look at `FloatingTileFactoryMethods.cpp` → `registerFrontendPanelTypes()`.

**Output format for Step 1:**

Record properties in this format (to be consumed by Step 2):

```markdown
### {ContentType} JSON Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| ProcessorId | String | "" | The ID of the connected module |
| Index | int | 0 | The visualisation type or parameter index |
| Font | String | "" | Font family name |
| FontSize | float | 14.0 | Font size in points |
| ... | ... | ... | ... |
```

### Step 2: Author the reference page

Write the MDC markdown file following the page template below. Work through each section:

1. **Frontmatter** — fill in all fields including `llmRef`
2. **Overview** — write 1-2 paragraphs from old docs. What this floating tile shows, when to use it, what module it connects to (if applicable).
3. **JSON Properties** — from Step 1 extraction + old docs descriptions.
4. **LAF Customisation** (if applicable) — author minimal LAF example, include `obj` property table.
5. **CSS Styling** (if applicable) — document selectors, pseudo-states, variables, example.
6. **Notes** — non-obvious behaviours, tips.
7. **See also** — placeholder for cross-reference post-processing.

### Step 3: Source completeness check

After writing the initial reference page, re-read all source material files and check for missing information:

- **Old docs (phase3)** — property descriptions, usage patterns, CSS examples, interaction quirks not yet on the page → add them.
- **Phase2 code examples** — `ScriptFloatingTile` has a phase2 directory (`enrichment/phase2/ScriptFloatingTile/`) with `setContentData.md` and `setLocalLookAndFeel.md`. Distill real-world usage patterns and gotchas into prose or Notes. Do not include verbatim.
- **MCP snippets** — if a snippet demonstrates a non-obvious behaviour for this content type, add the insight as a Note.
- **`custom_lookandfeel.md`** — check for LAF-related behavioural notes not yet captured.
- **`laf_style_guide.json`** — verify all `obj` properties are documented and no function is missing.

**Skip** information that lives purely in the scripting API docs — the cross-reference audit will link to it.

Focus on: JSON property interaction effects, LAF behavioural quirks (e.g., dual-call patterns), CSS setup requirements, and content-type-specific gotchas.

### Step 3b: Forum community insights

Follow the three-phase workflow in `style-guide/forum-insights-guide.md`:

1. `forum-search.py search "{ContentType}" --also "{natural name}" "{feature term}"`
2. Triage and fetch high-signal topics
3. `@extract-forum-insights` for structured insight extraction
4. `@verify-forum-claim` for bug/behaviour claims

**Target:** 2-4 new blocks per page (mix of warnings and tips). Floating tiles with no LAF/CSS may yield fewer insights.

### Step 4: Verify against gate checklist

Run through the gate checklist from `ui-component-enrichment.md`. Fix any gaps.

---

## Page Template (Track B)

```markdown
---
title: "{ContentType}"
description: "{1-2 sentence description for SEO and page metadata}"
contentType: "{ContentType}"
componentType: "floating-tile"
screenshot: "/images/v2/reference/ui-components/floating-tiles/{lowercase-contenttype}.png"
llmRef: |
  {ContentType} (FloatingTile)
  ContentType string: "{ContentType}"
  Set via: FloatingTile.set("ContentType", "{ContentType}")

  {1-2 sentence description.}

  JSON Properties:
    {property}: {brief description}
    ...

  Customisation:
    LAF: {function names or "none"}
    CSS: {selector or "none"}
seeAlso: []
commonMistakes:
  - title: "Descriptive title"
    wrong: "What users do incorrectly"
    right: "What they should do instead"
    explanation: "Why this matters"
---

![{ContentType}](/images/v2/reference/ui-components/floating-tiles/{lowercase-contenttype}.png)

{Overview prose: 1-2 paragraphs. What this floating tile displays,
what module it connects to, when to use it.}

## Setup

{Brief code example showing how to configure this floating tile:}

```javascript
const var ft = Content.getComponent("FloatingTile1");

ft.set("ContentType", "{ContentType}");
ft.set("Data", JSON.stringify({
    "ProcessorId": "ModuleName1",
    "Index": 0
}));
` ``

## JSON Properties

Configure via the `Data` property as a JSON string, or set individual properties in the Interface Designer.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `ProcessorId` | String | `""` | The ID of the connected module |
| ... | ... | ... | From Step 1 C++ extraction |

### ColourData sub-table

Most floating tiles support colour configuration via a nested `ColourData` JSON object. Document these in a separate sub-table after the main properties table:

```markdown
The `ColourData` object supports these colour IDs:

| Colour ID | Description |
|-----------|-------------|
| `bgColour` | Background colour |
| `textColour` | Text colour |
| `itemColour1` | Primary accent colour |
| `itemColour2` | Secondary accent colour |
```

The colour IDs correspond to the standard component colour properties and are passed within the `Data` JSON:

```javascript
ft.set("Data", JSON.stringify({
    "ProcessorId": "MyModule",
    "ColourData": {
        "bgColour": "0xFF333333",
        "textColour": "0xFFFFFFFF"
    }
}));
```

## LAF Customisation

{Only include this section if the content type has LAF functions.}

Register a custom look and feel to control the rendering of this floating tile.

### LAF Functions

| Function | Description |
|----------|-------------|
| `drawFunctionName` | Brief description from laf_style_guide.json |

### `obj` Properties

If 3+ LAF functions share >50% of their `obj` properties, use the coalesced format:
1. A **shared properties** table listing properties common to all functions
2. **Per-function tables** listing only the additional properties unique to each function

If only 2 functions or properties differ substantially, use separate tables per function.

Example (coalesced format):

```markdown
### `obj` Properties (shared across all functions)

| Property | Type | Description |
|----------|------|-------------|
| `obj.area` | Array[x,y,w,h] | The component bounds |
| `obj.bgColour` | int (ARGB) | Background colour |
| ... | ... | ... |

### Additional `obj` properties per function

`drawBackground` uses only the shared properties above.

#### `drawPath`

| Property | Type | Description |
|----------|------|-------------|
| `obj.path` | Path | The path to render |
| `obj.isActive` | bool | Whether this is the active section |
```

### Example

```javascript
const var ft = Content.getComponent("FloatingTile1");
const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawFunctionName", function(g, obj)
{
    // Draw background
    g.setColour(obj.bgColour);
    g.fillRect(obj.area);

    // Draw content using obj properties
    // ...
});

ft.setLocalLookAndFeel(laf);
` ``

## CSS Styling

{Only include this section if the content type has documented CSS support.}

### Selectors

| Selector | Description |
|----------|-------------|
| `.classname` | Targets the {content type} element |

### Pseudo-states

{If applicable.}

### CSS Variables

{If applicable — e.g., `--filterPath`, `--keyColour`.}

### Example Stylesheet

```javascript
const var ft = Content.getComponent("FloatingTile1");
const var laf = Content.createLocalLookAndFeel();

laf.setInlineStyleSheet("
.selector
{
    background: #333;
    // ...
}
");

ft.setLocalLookAndFeel(laf);
` ``

## Notes

{Non-obvious behaviours, practical tips. Draw from old docs.}

**See also:** {placeholder — populated during cross-reference post-processing}
```

---

## Input Manifests

### PresetBrowser

| Input | File |
|-------|------|
| JSON properties | C++ extraction from `PresetBrowserPanel` |
| LAF obj properties | `laf_style_guide.json` → `FloatingTileContentTypes.PresetBrowser` |
| LAF examples | `custom_lookandfeel.md` § PresetBrowser (7 functions) |
| CSS reference | `css_component_mapping.md` § PresetBrowser |
| CSS diagrams | `resources/images/extra/preset_css.png`, `preset_modal_css.png` |
| Old docs | `phase3/floating-tiles/presetbrowser.md` |
| Screenshot | `resources/images/floating-tiles/presetbrowser.png` |

**Notes:** Most complex floating tile. Seven LAF functions. Full CSS support with multiple sub-component selectors (see CSS diagrams). The modal popup has its own `.modal` class selector. Document `ShowFolderButton`, `ShowSaveButton`, `ShowNotes`, `ShowEditButtons`, `ShowFavoriteIcon`, `NumColumns` properties. Cross-references user preset system.

---

### Keyboard

| Input | File |
|-------|------|
| JSON properties | C++ extraction from `MidiKeyboardPanel` |
| LAF obj properties | `laf_style_guide.json` → `FloatingTileContentTypes.Keyboard` |
| LAF examples | `custom_lookandfeel.md` — (no keyboard LAF examples) |
| CSS reference | `css_component_mapping.md` § Keyboard |
| CSS example | `phase3/floating-tiles/keyboard.md` § CSS Styling (extensive) |
| Old docs | `phase3/floating-tiles/keyboard.md` |
| Screenshot | `resources/images/floating-tiles/keyboard.png` |

**Notes:** Three LAF functions (background, white note, black note). Rich CSS with `.keyboard`, `.whitekey`, `.blackkey` selectors and `--keyColour` / `--noteName` variables. Requires `CustomGraphics = false` and `DefaultAppearance = false` for CSS. Many JSON properties (KeyWidth, LowKey, HighKey, MPEKeyboard, BlackKeyRatio, etc.). The old docs contain an excellent CSS example.

---

### AHDSRGraph

| Input | File |
|-------|------|
| JSON properties | C++ extraction from `AhdsrEnvelope::Panel` |
| LAF obj properties | `laf_style_guide.json` → `FloatingTileContentTypes.AHDSRGraph` |
| LAF examples | `custom_lookandfeel.md` § AHDSR Graph (drawAhdsrPath, drawAhdsrBall) |
| Old docs | `phase3/floating-tiles/ahdsrgraph.md` |
| Screenshot | `resources/images/floating-tiles/ahdsrgraph.png` |

**Notes:** Three LAF functions (background, ball, path). The `drawAhdsrPath` is called twice (once for full path, once for active section — distinguished by `obj.isActive`). The `currentState` property indicates which envelope stage is active. Connect via `ProcessorId`.

---

### FlexAHDSRGraph

| Input | File |
|-------|------|
| JSON properties | C++ extraction from `FlexAhdsrEnvelope::Panel` |
| LAF obj properties | `laf_style_guide.json` → `FloatingTileContentTypes.FlexAHDSRGraph` |
| LAF examples | — (no existing examples) |
| Old docs | — (no old docs) |
| Screenshot | — (no screenshot) |

**Notes:** Six LAF functions (background, curve point, full path, position, segment, text). Newer component with no old docs or examples. The LAF example will need to be authored from scratch using `laf_style_guide.json` obj properties.

---

### AudioAnalyser

| Input | File |
|-------|------|
| JSON properties | C++ extraction from `AudioAnalyserComponent::Panel` |
| LAF obj properties | `laf_style_guide.json` → `FloatingTileContentTypes.AudioAnalyser` |
| LAF examples | — (no existing examples) |
| Old docs | `phase3/floating-tiles/audioanalyser.md` |
| Screenshot | `resources/images/floating-tiles/audioanalyser.png` |

**Notes:** Three LAF functions (background, path, grid). Three visualisation modes via `Index`: Goniometer (0), Oscilloscope (1), Spectral Analyser (2). Connect via `ProcessorId` to an Analyser module. The Plotter floating tile reuses the same LAF functions.

---

### FilterDisplay

| Input | File |
|-------|------|
| JSON properties | C++ extraction from `FilterGraph::Panel` |
| LAF obj properties | `laf_style_guide.json` → `FloatingTileContentTypes.FilterDisplay` |
| LAF examples | — (no existing examples) |
| CSS reference | `css_component_mapping.md` § FilterDisplay |
| Old docs | `phase3/floating-tiles/filterdisplay.md` |
| Screenshot | `resources/images/floating-tiles/filterdisplay.png` |

**Notes:** Three LAF functions (background, path, grid lines). CSS uses `.filtergraph` selector with `--filterPath` variable. Connect via `ProcessorId` and `Index`.

---

### DraggableFilterPanel

| Input | File |
|-------|------|
| JSON properties | C++ extraction from `FilterDragOverlay::Panel` |
| LAF obj properties | `laf_style_guide.json` → `FloatingTileContentTypes.DraggableFilterPanel` |
| LAF examples | — (no existing examples) |
| LAF snippets | MCP: `draggable-filter-example`, `filterdisplay-pathtype-example` |
| CSS reference | `css_component_mapping.md` § DraggableFilterPanel |
| Old docs | `phase3/floating-tiles/draggablefilterpanel.md` |
| Screenshot | `resources/images/floating-tiles/draggablefilterpanel.png` |

**Notes:** One LAF function (`drawFilterDragHandle`). Rich CSS with `.filtergraph` + `.filterHandle` selectors and multiple CSS variables (`--type`, `--frequency`, `--q`, `--gain`, `--index`). Many JSON properties (PathType, GainRange, AllowFilterResizing, etc.). Document the 4 PathType options (see `resources/images/extra/filterpathtypes.png`).

---

### Waveform

| Input | File |
|-------|------|
| JSON properties | C++ extraction from `WaveformComponent::Panel` |
| LAF obj properties | `laf_style_guide.json` → `FloatingTileContentTypes.Waveform` |
| Old docs | `phase3/floating-tiles/waveform.md` |
| Screenshot | `resources/images/floating-tiles/waveform.png` |

**Notes:** Two LAF functions (background, path). Connects to SineWave Generator or Waveform Generator via `ProcessorId`. Simple floating tile.

---

### WavetableWaterfall

| Input | File |
|-------|------|
| JSON properties | C++ extraction from `WaterfallComponent::Panel` |
| Old docs | `phase3/floating-tiles/wavetablewaterfall.md` (if it exists) |
| Screenshot | `resources/images/floating-tiles/wavetablewaterfall.png` |

**Notes:** No LAF, no CSS. Displays a 3D waterfall view of wavetable content. Check old docs for any available content.

---

### MatrixPeakMeter

| Input | File |
|-------|------|
| JSON properties | C++ extraction from `MatrixPeakMeter` |
| LAF obj properties | `laf_style_guide.json` → `FloatingTileContentTypes.MatrixPeakMeter` |
| LAF snippets | MCP: `peak-meter-laf-with-filmstrip-image` |
| Screenshot | `resources/images/floating-tiles/matrixpeakmeter.png` |

**Notes:** Single LAF function (`drawMatrixPeakMeter`). No CSS. The `peak-meter-laf-with-filmstrip-image` snippet is a good starting point.

---

### ModulationMatrix

| Input | File |
|-------|------|
| JSON properties | C++ extraction |
| LAF obj properties | `laf_style_guide.json` → `FloatingTileContentTypes.ModulationMatrix` |
| Screenshot | `resources/images/floating-tiles/modulationmatrix.PNG` |

**Notes:** Three LAF functions (getModulatorDragData, drawModulationDragBackground, drawModulationDragger). No CSS, no old docs. Part of the matrix modulation system introduced in HISE 5.0.

---

### ModulationMatrixController

| Input | File |
|-------|------|
| JSON properties | C++ extraction |
| Screenshot | `resources/images/floating-tiles/modulationmatrixcontroller.png` |

**Notes:** No LAF, no CSS, no old docs. Companion to the ModulationMatrix.

---

### Plotter

| Input | File |
|-------|------|
| JSON properties | C++ extraction |
| LAF obj properties | Uses Analyser LAF functions (drawAnalyserBackground, drawAnalyserPath) |
| Old docs | `phase3/floating-tiles/plotter.md` |
| Screenshot | `resources/images/floating-tiles/plotter.png` |

**Notes:** Reuses AudioAnalyser LAF functions but for modulation signals rather than audio. `drawAnalyserGrid` is never called for the Plotter (noted in old docs). Connect via `ProcessorId` to a modulator.

---

### Remaining 14 content types (no LAF)

These content types have no LAF functions and no CSS support. Their pages are thin — primarily JSON properties + overview prose.

| Content Type | Old docs | Screenshot | Key notes |
|-------------|----------|------------|-----------|
| CustomSettings | Yes | Yes | Audio device settings panel |
| PerformanceLabel | Yes | Yes | CPU/voice count display |
| ActivityLed | Yes | Yes | Audio activity indicator |
| TooltipPanel | Yes | Yes | Shows component tooltips |
| MidiOverlayPanel | Yes | Yes | MIDI file drag & drop |
| MidiSources | Yes | Yes | MIDI input selector |
| MidiChannelList | Yes | Yes | MIDI channel selector |
| MidiLearnPanel | Yes | Yes | MIDI learn assignment display |
| FrontendMacroPanel | Yes | Yes | Macro control sliders |
| MPEPanel | Yes | Yes | MPE configuration |
| AboutPagePanel | — | — | About/credits page |
| MarkdownPanel | — | Yes | Renders markdown content |
| Empty | — | — | Empty placeholder |

For each of these:
1. Extract JSON properties from C++ (Step 1)
2. Write a short overview from old docs
3. Include JSON property table
4. Skip LAF and CSS sections
5. Add notes if relevant

---

## Batching Strategy

Process in the order specified in `ui-component-enrichment.md`:

| Batch | Content types | C++ files |
|-------|--------------|-----------|
| 1 (LAF-rich) | PresetBrowser, Keyboard, AHDSRGraph | FrontendPanelTypes.h, AhdsrEnvelope.h |
| 2 (LAF-medium) | FilterDisplay, DraggableFilterPanel, AudioAnalyser, FlexAHDSRGraph | FrontendPanelTypes.h, FlexAhdsrEnvelope.h |
| 3 (LAF-simple) | Waveform, MatrixPeakMeter, ModulationMatrix, Plotter | FrontendPanelTypes.h |
| 4 (no LAF) | All remaining 15 | FrontendPanelTypes.h + various |

Within each batch, the C++ property extraction (Step 1) can be done for all content types in the batch simultaneously, since they often share the same source file.
