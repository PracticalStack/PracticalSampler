# UI Component Enrichment Pipeline

**Purpose:** Produce reference pages for HISE UI components (plugin components and floating tile content types) as MDC markdown for the Nuxt.js docs site and `llmRef` text for the MCP server. Each page brings together property documentation, visual customisation (LAF + CSS), and usage guidance into a single authoritative reference.

**Output locations:**
- `ui_enrichment/pages/components/{ComponentName}.md` — Plugin component reference pages
- `ui_enrichment/pages/floating-tiles/{ContentType}.md` — Floating tile reference pages

**Sub-track details:**
- `ui-component-enrichment/track-a-plugin-components.md` — Track A: Plugin component authoring guide
- `ui-component-enrichment/track-b-floating-tiles.md` — Track B: Floating tile authoring guide

---

## Strategic Context: UI Components vs. Scripting API vs. Modules

The UI component enrichment pipeline runs parallel to two other pipelines but addresses a fundamentally different documentation surface.

| Dimension | Scripting API | Modules | UI Components |
|-----------|---------------|---------|---------------|
| Unit of work | Method on a class | Audio processor | Visual component / content type |
| Source of truth | C++ method body | `processBlock` + module metadata | Property registration + LAF/CSS rendering code |
| Shape of knowledge | Parameters, return values, thread safety | Signal flow topology, parameter interactions | Properties, visual customisation, interaction model |
| Visual output | None (text only) | Interactive pseudo-code | LAF example + CSS example + screenshot |
| User question | "What does this method do?" | "How does this module process audio?" | "How do I use and style this component?" |
| Agent judgment | Low (mostly mechanical extraction) | High (topology authoring) | Medium (LAF example authoring, CSS verification) |

### Relationship to scripting API enrichment

The scripting API pipeline already enriches the `ScriptSlider`, `ScriptButton`, etc. classes as API namespaces — their methods (`setValue()`, `setRange()`, `setLocalLookAndFeel()`, etc.) are fully documented in phase4b. The UI component pipeline focuses on **what the component is and how it looks**, not how to call its methods:

- **Properties** (what you pass to `set()`) → UI component page
- **Methods** (what you call on the component) → Scripting API docs
- **Visual customisation** (LAF, CSS, filmstrips) → UI component page
- **Interaction model** (for ScriptPanel: callbacks, data model) → Scripting API docs

Cross-references between the two surfaces are handled by the post-processing step (`cross-reference-audit.md`).

---

## Pipeline Overview

```
PREPARATION (one-shot, already complete)
  ├── CSS per-component mapping survey
  │     -> ui_enrichment/resources/css_component_mapping.md
  └── ScriptPanel content triage
        -> ui_enrichment/resources/scriptpanel_ui.md (for this pipeline)
        -> ui_enrichment/resources/scriptpanel_api.md (for scripting API backport)

TRACK A: Plugin Components (11 types, pure authoring)
  Input per component:
    - phase4b set.md (property table)
    - laf_style_guide.json (LAF obj properties, if applicable)
    - css_component_mapping.md (CSS selectors/states/variables)
    - custom_lookandfeel.md + MCP snippets (LAF example source material)
    - phase3 old docs (prose, CSS examples)
    - screenshot from resources/images/
    - scriptpanel_ui.md (ScriptPanel only)
    - HISE forum API search results (Step 3b — queried during authoring)
  Output: reference page (MDC markdown + llmRef frontmatter)

TRACK B: Floating Tiles (26 content types, one C++ step + authoring)
  Step 1: Extract JSON config properties from C++ panel class
  Step 2: Author reference page using:
    - Extracted JSON properties
    - laf_style_guide.json (if applicable)
    - css_component_mapping.md (if applicable)
    - custom_lookandfeel.md + MCP snippets (LAF example source material)
    - phase3 old docs (prose, CSS examples)
    - screenshot from resources/images/
  Output: reference page (MDC markdown + llmRef frontmatter)
```

---

## Component Inventory

### Track A: Plugin Components

| Component | LAF functions | CSS | Special notes |
|-----------|--------------|-----|---------------|
| ScriptSlider | `drawRotarySlider`, `drawLinearSlider` | `.scriptslider` | Matrix modulation integration |
| ScriptButton | `drawToggleButton` | `button` | Radio groups, filmstrip |
| ScriptComboBox | `drawComboBox` | `select` | Custom popup syntax |
| ScriptLabel | — | `label`, `input` | Text input styling |
| ScriptPanel | — (uses paint routine) | `div` | Most complex; see `scriptpanel_ui.md` |
| ScriptTable | 5 LAF functions | `.scripttable` | Mouse handling properties |
| ScriptSliderPack | 4 LAF functions | `.scriptsliderpack` | Step sequencer flash |
| ScriptAudioWaveform | 7 LAF functions | `.scriptaudiowaveform` | AudioSampleProcessor connection |
| ScriptImage | — | `img` (minimal) | Simplest component |
| ScriptFloatingTile | — | `div` | Wrapper; indexes 26 content types |
| ScriptedViewport | — | `.scriptviewport` | List mode with `tr` rows |
| ScriptDynamicContainer | — | — | Dynamic layout container; no visual representation |

**Excluded:** ScriptMultipageDialog (deprecated)

### Track B: Floating Tiles (26 content types)

| Content Type | LAF | CSS | Old docs |
|-------------|-----|-----|----------|
| PresetBrowser | 7 functions | Yes (complex) | Yes |
| Keyboard | 3 functions | Yes | Yes |
| AHDSRGraph | 3 functions | — | Yes |
| FlexAHDSRGraph | 6 functions | — | — |
| AudioAnalyser | 3 functions | — | Yes |
| FilterDisplay | 3 functions | `.filtergraph` | Yes |
| DraggableFilterPanel | 1 function | `.filtergraph`, `.filterHandle` | Yes |
| Waveform | 2 functions | — | Yes |
| WavetableWaterfall | — | — | Yes |
| MatrixPeakMeter | 1 function | — | — |
| ModulationMatrix | 3 functions | — | — |
| ModulationMatrixController | — | — | — |
| Plotter | uses Analyser LAF | — | Yes |
| CustomSettings | — | — | Yes |
| PerformanceLabel | — | — | Yes |
| ActivityLed | — | — | Yes |
| TooltipPanel | — | — | Yes |
| MidiOverlayPanel | — | — | Yes |
| MidiSources | — | — | Yes |
| MidiChannelList | — | — | Yes |
| MidiLearnPanel | — | — | Yes |
| FrontendMacroPanel | — | — | Yes |
| MPEPanel | — | — | Yes |
| AboutPagePanel | — | — | — |
| MarkdownPanel | — | — | — |
| Empty | — | — | — |

---

## Input Data Sources

| Data | Source file | Status |
|------|-----------|--------|
| Property tables (ScriptComponents) | `enrichment/phase4b/{ClassName}/set.md` | Complete |
| Property tables (ScriptDynamicContainer) | `enrichment/phase4b/ScriptDynamicContainer/set.md` | Complete |
| FloatingTile JSON config properties | C++ panel classes (extracted per Track B Step 1) | Per-item |
| LAF `obj` properties | `hi_scripting/scripting/api/laf_style_guide.json` | Complete |
| LAF code examples | `ui_enrichment/resources/custom_lookandfeel.md` + MCP snippets | ~80% coverage |
| CSS selector mapping | `ui_enrichment/resources/css_component_mapping.md` | Complete |
| CSS examples | `ui_enrichment/phase3/plugin-components/*.md` + `phase3/floating-tiles/*.md` | ~8 components |
| Old docs prose | `ui_enrichment/phase3/plugin-components/*.md` + `phase3/floating-tiles/*.md` | 10 + ~20 |
| ScriptPanel UI content | `ui_enrichment/resources/scriptpanel_ui.md` | Complete |
| Screenshots | `ui_enrichment/resources/images/plugin-components/` + `floating-tiles/` | 11 + 25 |
| LAF → component mappings | `doc_builders/laf-extraction.md` (hardcoded tables) | Complete |
| FloatingTile content type list | MCP server `ScriptFloatingTile.ContentType` (26 values) | Complete |
| CSS general reference | `ui_enrichment/resources/css.md` | Complete |
| Property name reference | `ui_enrichment/resources/component_properties_reference.md` | Complete — authoritative casing/defaults from runtime |
| Phase2 code examples | `enrichment/phase2/{ClassName}/` | Available for 10 of 12 ScriptComponent classes (not ScriptButton, ScriptDynamicContainer) |

---

## Directory Structure

```
tools/api generator/
  doc_builders/
    ui-component-enrichment.md                  # This file (pipeline orchestrator)
    ui-component-enrichment/
      track-a-plugin-components.md              # Track A authoring guide
      track-b-floating-tiles.md                 # Track B authoring guide

  ui_enrichment/
    phase3/                                     # Old docs (read-only input)
      plugin-components/
        button.md, knob.md, panel.md, ...       # 10 component pages
        Readme.md                               # Common properties
      floating-tiles/
        presetbrowser.md, keyboard.md, ...      # ~20 content type pages
    resources/                                  # Preparation outputs + reference material
      css.md                                    # CSS general reference
      css_component_mapping.md                  # CSS per-component survey
      custom_lookandfeel.md                     # LAF examples from old docs
      scriptpanel.md                            # Old ScriptPanel guide (raw)
      scriptpanel_ui.md                         # ScriptPanel triage: UI page content
      scriptpanel_api.md                        # ScriptPanel triage: API backport
      images/
        plugin-components/                      # 11 screenshots
        floating-tiles/                         # 25 screenshots
        extra/                                  # CSS diagrams, filter types, etc.
    pages/                                      # Final output
      components/
        {ComponentName}.md                      # One per plugin component
      floating-tiles/
        {ContentType}.md                        # One per floating tile content type
```

---

## Reference Page Template

Both tracks produce pages following this structure. The full template with all sections is in each track's guide.

```markdown
---
title: Component Display Name
description: 1-2 sentence description for SEO and page metadata
componentId: ScriptSlider
componentType: plugin-component | floating-tile
category: plugin-component | floating-tile
screenshot: /images/v2/reference/ui-components/scriptslider.png
llmRef: |
  [concise LLM reference text — see format below]
seeAlso:
  - { id: "...", type: "api|floating_tile|component|module", reason: "..." }
commonMistakes:
  - title: "Descriptive title"
    wrong: "What users do incorrectly"
    right: "What they should do instead"
    explanation: "Why this matters"
---

[Overview prose]

## Properties
[Property table]

## LAF Customisation
[LAF example + obj property table]

## CSS Styling
[CSS selectors, pseudo-states, variables, example]

## Notes
[Non-obvious behaviours, tips]

**See also:** [cross-reference links]
```

---

## `llmRef` Format

The `llmRef` frontmatter field contains a concise, structured text blob optimised for LLM consumption by the MCP server. It follows the phase4b style but is adapted for UI component concepts.

### Plugin Component `llmRef` format

```
{ComponentName} (UI component)
Create via: Content.addXxx("name", x, y)
Scripting API: $API.{ClassName}$

{1-2 sentence description of what the component is and does.}

Properties (component-specific):
  {property}: {brief description}
  ...

Customisation:
  LAF: {function names, or "paint routine" for Panel, or "none"}
  CSS: {selector} with {pseudo-states}
  Filmstrip: {yes/no}
```

> **Note:** Common mistakes are NOT included in `llmRef`. They are stored as structured YAML in the top-level `commonMistakes` frontmatter field (title/wrong/right/explanation format), which `publish.py` converts to the `::common-mistakes` MDC component.

### Floating Tile `llmRef` format

```
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
```

---

## LAF Example Authoring Guide

Applicable to both tracks. When a component or content type has LAF functions, the reference page includes a minimal HiseScript example.

### What "minimal replication" means

The LAF example should:
1. **Be recognisable** as the component — someone should look at it and know what it's drawing
2. **Demonstrate all key `obj` properties** — show how to use `obj.area`, `obj.value`, state flags (`obj.hover`, `obj.down`, etc.), and colour properties
3. **Be self-contained** — runnable as a snippet with no external dependencies (images, fonts)
4. **Be concise** — 20-50 lines per function, not a production-ready skin

The LAF example should NOT:
- Pixel-perfectly recreate the stock HISE skin
- Include elaborate styling, gradients, or animations
- Use external images or custom fonts
- Cover edge cases or advanced features (modulation display, etc.)

### Code style

- Use `Content.createLocalLookAndFeel()` (not global)
- Use `laf.registerFunction("functionName", function(g, obj) { ... })` syntax
- Reference `obj` properties by name with brief inline comments
- Use the component's colour properties (`obj.bgColour`, `obj.itemColour1`, etc.)
- End with `component.setLocalLookAndFeel(laf)`

### Source material priority

1. **`laf_style_guide.json`** — authoritative for `obj` property names and types
2. **`custom_lookandfeel.md`** — existing examples, may need updating
3. **MCP snippets** — community examples, use for inspiration but simplify
4. **Phase3 old docs** — may have examples in the CSS section

### Components with multiple LAF functions

For components with multiple LAF functions (e.g., ScriptTable with 5 functions), include all functions in a single example block. Group them logically and add comments explaining what each function draws.

---

## Gate Checklist (applies to both tracks)

Before marking a component/content type as complete, verify:

- [ ] **Property table complete** — all properties from phase4b `set.md` (Track A) or C++ extraction (Track B) are in the table
- [ ] **LAF section complete** (if applicable) — minimal example present, all `obj` properties listed in table, function descriptions accurate
- [ ] **CSS section complete** (if applicable) — selectors, pseudo-states, variables, and example present; verified against `css_component_mapping.md`
- [ ] **Screenshot referenced** — correct image path from `resources/images/`
- [ ] **Overview prose** — 1-3 paragraphs, no C++ internals, accurate description of what the component does
- [ ] **`llmRef` frontmatter** — present, follows the format above, concise, does NOT contain common mistakes (those go in `commonMistakes` frontmatter)
- [ ] **`commonMistakes` frontmatter** — present with at least 1 entry using title/wrong/right/explanation format
- [ ] **LAF `obj` tables coalesced** (if applicable) — if 3+ LAF functions share >50% of `obj` properties, use a shared properties table + per-function additional properties tables. If only 2 functions or properties differ substantially, keep separate tables per function.
- [ ] **Source completeness verified** — old docs, phase2 code examples, and snippet insights reviewed; relevant information incorporated into prose, property descriptions, or Notes
- [ ] **Forum community insights scanned** — HISE forum searched for recurring confusion and best practices; insights added as Warning/Tip blocks scattered throughout the page (see Track A Step 3b and `style-guide/canonical-links.md` § 5)
- [ ] **No C++ leakage** — prose uses HiseScript terminology, not C++ class names or implementation details
- [ ] **Cross-references deferred** — no inline `$DOMAIN.Target$` tokens yet (added in post-processing)
- [ ] **Deactivated properties noted** — if the component deactivates base properties, listed in a "Deactivated properties" note

---

## Batching Strategy

### Track A: Plugin Components

Process in complexity order so that simpler pages establish the format before tackling complex ones:

| Batch | Components | Rationale |
|-------|-----------|-----------|
| 1 (simple) | ScriptImage, ScriptLabel | Minimal LAF/CSS, establishes basic format |
| 2 (standard) | ScriptButton, ScriptComboBox, ScriptedViewport | Standard LAF + CSS |
| 3 (complex) | ScriptSlider, ScriptTable, ScriptSliderPack, ScriptAudioWaveform | Multiple LAF functions, rich CSS |
| 4 (special) | ScriptPanel, ScriptFloatingTile, ScriptDynamicContainer | Unique documentation needs |

### Track B: Floating Tiles

Process by LAF complexity:

| Batch | Content types | Rationale |
|-------|--------------|-----------|
| 1 (LAF-rich) | PresetBrowser, Keyboard, AHDSRGraph | Most complex LAF + CSS, old docs available |
| 2 (LAF-medium) | FilterDisplay, DraggableFilterPanel, AudioAnalyser, FlexAHDSRGraph | Multiple LAF functions |
| 3 (LAF-simple) | Waveform, MatrixPeakMeter, ModulationMatrix, Plotter | 1-3 LAF functions |
| 4 (no LAF) | All remaining 15 content types | JSON properties only, thin pages |
