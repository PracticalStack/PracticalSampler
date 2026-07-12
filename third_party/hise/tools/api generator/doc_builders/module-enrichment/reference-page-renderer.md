# Reference Page Renderer - Nuxt.js Component Spec

**Purpose:** Specification for the three custom Vue components that render MDC markdown module reference pages. These components are consumed by `::signal-path`, `::parameter-table`, and `::modulation-table` blocks in the authored markdown.

**Design reference:** `module_enrichment/resources/reference/` (5 HTML prototypes)

---

## Theme Constants

All components share the HISE dark theme. Define these as CSS custom properties at the site level:

```css
:root {
  /* Backgrounds */
  --hise-bg:            #0d1117;
  --hise-bg-raised:     #161b22;
  --hise-bg-hover:      #1a1f27;
  --hise-bg-table:      #131920;

  /* Borders */
  --hise-border:        #21262d;
  --hise-border-dark:   rgba(0, 0, 0, 0.5);

  /* Text */
  --hise-text:          #ccc;
  --hise-text-heading:  #e2e8f0;
  --hise-text-muted:    #808080;
  --hise-text-subtle:   #909aab;
  --hise-text-prose:    #b0b8c4;

  /* Glossary highlight colours */
  --hise-param:         #4a9eff;
  --hise-param-dot:     rgba(74, 158, 255, 0.4);
  --hise-func:          #f97316;
  --hise-func-dot:      rgba(249, 115, 22, 0.4);
  --hise-mod:           #90FFB1;
  --hise-mod-dot:       rgba(144, 255, 177, 0.4);

  /* Typography */
  --hise-font-body:     Lato, sans-serif;
  --hise-font-mono:     Consolas, 'Courier New', monospace;
}
```

---

## 1. `SignalPath.vue`

Renders the interactive pseudo-code block with glossary-based syntax highlighting and hover tooltips.

### MDC Usage

```markdown
::signal-path
---
glossary:
  parameters:
    Attack:
      desc: "Time to reach full level after note-on"
      range: "0 - 20000 ms"
      default: "20 ms"
  functions:
    expRamp:
      desc: "Exponential ramp between two levels over a given time"
  modulations:
    AttackTimeMod:
      desc: "Scales attack time per voice at note-on"
      scope: "per-voice"
---

```
// AHDSR - per-voice envelope modulator
onNoteOn() {
    attackTime = Attack * AttackTimeMod;
    output = expRamp(0, peak, attackTime, AttackCurve);
}
```

::
```

### Props

| Prop | Type | Required | Description |
|------|------|----------|-------------|
| `glossary` | `Object` | Yes | Three-category glossary object (see below) |

### Glossary Schema

```typescript
interface Glossary {
  parameters?: Record<string, {
    desc: string;
    range?: string;
    default?: string;
  }>;
  functions?: Record<string, {
    desc: string;
  }>;
  modulations?: Record<string, {
    desc: string;
    scope?: string;  // "per-voice" | "global"
  }>;
}
```

### Slot Content

The default slot receives the pseudo-code as a fenced code block. The component extracts the raw text from the rendered slot content and applies its own highlighting - it does NOT use the default markdown code renderer.

### Highlighting Engine

The highlighting algorithm runs client-side on mount:

1. **Build lookup table.** Flatten all three glossary categories into a single map: `key -> { type, desc, range?, default?, scope? }`. Type is derived from the category name (`parameters` -> `param`, `functions` -> `func`, `modulations` -> `mod`).

2. **Sort keys longest-first.** This prevents partial matches (e.g. `AttackLevel` must match before `Attack`).

3. **Process line by line:**
   - If the trimmed line starts with `//`, wrap the entire line in `<span class="sp-comment">`.
   - Otherwise, split at the first `//` to separate code from inline comment.
   - HTML-escape the code portion.
   - For each glossary key (longest-first), replace `\bKEY\b` occurrences with `<span class="sp-{type}" data-key="KEY">KEY</span>`.
   - Replace language keywords (`if`, `else`, `return`, and callback names like `process`, `onNoteOn`, `onNoteOff`, `onMidiEvent`) with `<span class="sp-keyword">`.
   - Append the comment portion (if any) wrapped in `<span class="sp-comment">`.

4. **Join lines** with `\n` and set as `innerHTML` of the code container.

### Tooltip Behaviour

- **Trigger:** `mouseover` on any `[data-key]` span inside the code block.
- **Content:** Glossary entry name (coloured by type), description, and metadata line (range + default for parameters, scope for modulations).
- **Positioning:** Fixed position, follows cursor on `mousemove`. Clamped to viewport edges (max 340px width, 4px minimum top offset).
- **Dismiss:** `mouseout` from the `[data-key]` span hides the tooltip.
- **No click interaction.** Tooltip is `pointer-events: none`.

### Tooltip HTML Structure

```html
<div class="sp-tooltip">
  <div class="sp-tt-name sp-{type}">{key}</div>
  <div class="sp-tt-desc">{desc}</div>
  <div class="sp-tt-meta">
    <span>Range: {range}</span>    <!-- parameters only -->
    <span>Default: {default}</span> <!-- parameters only -->
    <span>Scope: {scope}</span>     <!-- modulations only -->
  </div>
</div>
```

### Legend

Below the code block, render a legend bar:

```html
<div class="sp-legend">
  <div class="sp-legend-item">
    <div class="sp-legend-dot" style="background: var(--hise-param)"></div>
    Parameter
  </div>
  <div class="sp-legend-item">
    <div class="sp-legend-dot" style="background: var(--hise-func)"></div>
    Function
  </div>
  <div class="sp-legend-item">
    <div class="sp-legend-dot" style="background: var(--hise-mod)"></div>
    Modulation
  </div>
</div>
```

Only show legend items for categories that have at least one glossary entry.

### Pseudo-Code Label

Above the code block, render a subtle label:

```html
<div class="sp-label">Pseudo-code - hover highlighted terms for details</div>
```

Style: `font-size: 11px; color: var(--hise-text-muted); margin-bottom: 6px;`

### Styles

```css
.signal-path {
  margin: 16px 0;
}

.sp-label {
  font-size: 11px;
  color: var(--hise-text-muted);
  margin-bottom: 6px;
}

.sp-code {
  background: var(--hise-bg-raised);
  border: 1px solid var(--hise-border-dark);
  border-radius: 6px;
  padding: 20px 24px;
  font-family: var(--hise-font-mono);
  font-size: 14px;
  line-height: 1.7;
  overflow-x: auto;
  white-space: pre;
  max-width: 720px;
}

.sp-comment { color: var(--hise-text-muted); }
.sp-keyword { color: #c678dd; }

.sp-param {
  color: var(--hise-param);
  cursor: help;
  border-bottom: 1px dotted var(--hise-param-dot);
}
.sp-func {
  color: var(--hise-func);
  cursor: help;
  border-bottom: 1px dotted var(--hise-func-dot);
}
.sp-mod {
  color: var(--hise-mod);
  cursor: help;
  border-bottom: 1px dotted var(--hise-mod-dot);
}

.sp-tooltip {
  position: fixed;
  display: none;
  background: #1e293b;
  border: 1px solid #334155;
  border-radius: 6px;
  padding: 10px 14px;
  max-width: 340px;
  font-family: var(--hise-font-body);
  font-size: 13px;
  line-height: 1.5;
  color: var(--hise-text);
  pointer-events: none;
  z-index: 100;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.4);
}
.sp-tt-name { font-weight: bold; margin-bottom: 4px; }
.sp-tt-name.sp-param { color: var(--hise-param); }
.sp-tt-name.sp-func  { color: var(--hise-func); }
.sp-tt-name.sp-mod   { color: var(--hise-mod); }
.sp-tt-desc { color: var(--hise-text); margin-bottom: 6px; }
.sp-tt-meta {
  font-size: 11px;
  color: var(--hise-text-muted);
  font-family: var(--hise-font-mono);
}
.sp-tt-meta span { margin-right: 12px; }

.sp-legend {
  display: flex;
  gap: 24px;
  margin-top: 16px;
  font-size: 12px;
  color: var(--hise-text-muted);
}
.sp-legend-item { display: flex; align-items: center; gap: 6px; }
.sp-legend-dot { width: 8px; height: 8px; border-radius: 50%; }
```

---

## 2. `ParameterTable.vue`

Renders a grouped parameter table with group header rows.

### MDC Usage

```markdown
::parameter-table
---
groups:
  - label: Envelope
    params:
      - { name: Attack, desc: "Time to reach full level after note-on", range: "0 - 20000 ms", default: "20 ms" }
      - { name: Hold, desc: "Time at attack level before decaying", range: "0 - 20000 ms", default: "10 ms" }
  - label: Curve Shape
    params:
      - { name: AttackCurve, desc: "Attack phase curvature", range: "0.0 - 1.0", default: "0.0" }
---
::
```

### Props

| Prop | Type | Required | Description |
|------|------|----------|-------------|
| `groups` | `Array<Group>` | Yes | Grouped parameter definitions |

```typescript
interface Group {
  label: string;
  params: Array<{
    name: string;
    desc: string;
    range: string;
    default: string;
  }>;
}
```

### Rendered Structure

```html
<table class="pt-table">
  <thead>
    <tr>
      <th>Parameter</th>
      <th>Description</th>
      <th>Range</th>
      <th>Default</th>
    </tr>
  </thead>
  <tbody>
    <!-- Repeat per group -->
    <tr class="pt-group-row">
      <td colspan="4">{group.label}</td>
    </tr>
    <!-- Repeat per param -->
    <tr>
      <td class="pt-name">{param.name}</td>
      <td>{param.desc}</td>
      <td class="pt-range">{param.range}</td>
      <td class="pt-default">{param.default}</td>
    </tr>
  </tbody>
</table>
```

### Styles

```css
.pt-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
  margin-top: 12px;
  background: var(--hise-bg-table);
  border: 1px solid var(--hise-border);
  border-radius: 6px;
  overflow: hidden;
}
.pt-table th {
  text-align: left;
  color: var(--hise-text-subtle);
  font-weight: 600;
  font-size: 11px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  padding: 8px 10px;
  background: var(--hise-bg-raised);
  border-bottom: 1px solid var(--hise-border);
}
.pt-table td {
  padding: 8px 10px;
  border-bottom: 1px solid var(--hise-bg-hover);
  color: var(--hise-text-prose);
  vertical-align: top;
}
.pt-table tr:hover td {
  background: var(--hise-bg-hover);
}
.pt-name {
  font-family: var(--hise-font-mono);
  color: var(--hise-param);
  white-space: nowrap;
}
.pt-range {
  font-family: var(--hise-font-mono);
  font-size: 12px;
  color: var(--hise-text-muted);
}
.pt-default {
  font-family: var(--hise-font-mono);
  font-size: 12px;
  color: var(--hise-text);
}
.pt-group-row td {
  padding-top: 16px !important;
  color: var(--hise-text-muted);
  font-size: 11px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  border-bottom: 1px solid var(--hise-border);
}
```

---

## 3. `ModulationTable.vue`

Renders a modulation chain table.

### MDC Usage

```markdown
::modulation-table
---
chains:
  - { name: AttackTimeMod, desc: "Scales attack time per voice at note-on", scope: "per-voice", constrainer: "VoiceStartModulator" }
  - { name: DecayTimeMod, desc: "Scales decay time per voice at note-on", scope: "per-voice", constrainer: "VoiceStartModulator" }
---
::
```

### Props

| Prop | Type | Required | Description |
|------|------|----------|-------------|
| `chains` | `Array<Chain>` | Yes | Modulation chain definitions |

```typescript
interface Chain {
  name: string;
  desc: string;
  scope: string;        // "per-voice" | "global"
  constrainer?: string; // e.g. "VoiceStartModulator", "TimeVariantModulator"
}
```

### Rendered Structure

```html
<table class="mt-table">
  <thead>
    <tr>
      <th>Chain</th>
      <th>Description</th>
      <th>Scope</th>
      <th>Constrainer</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td class="mt-name">{chain.name}</td>
      <td>{chain.desc}</td>
      <td class="mt-scope">{chain.scope}</td>
      <td>{chain.constrainer}</td>
    </tr>
  </tbody>
</table>
```

### Styles

```css
.mt-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
  margin-top: 12px;
  background: var(--hise-bg-table);
  border: 1px solid var(--hise-border);
  border-radius: 6px;
  overflow: hidden;
}
.mt-table th {
  text-align: left;
  color: var(--hise-text-subtle);
  font-weight: 600;
  font-size: 11px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  padding: 8px 10px;
  background: var(--hise-bg-raised);
  border-bottom: 1px solid var(--hise-border);
}
.mt-table td {
  padding: 8px 10px;
  border-bottom: 1px solid var(--hise-bg-hover);
  color: var(--hise-text-prose);
  vertical-align: top;
}
.mt-table tr:hover td {
  background: var(--hise-bg-hover);
}
.mt-name {
  font-family: var(--hise-font-mono);
  color: var(--hise-mod);
  white-space: nowrap;
}
.mt-scope {
  font-size: 11px;
  color: var(--hise-text-muted);
}
```

---

## 4. `CategoryTags.vue`

Renders category tags as pill-shaped badges below the page heading. Each tag links to the corresponding anchor on the categories index page and shows a tooltip with the category description on hover.

### MDC Usage

```markdown
::category-tags
---
tags:
  - { name: routing, desc: "Modules that forward, distribute, or proxy signals or events across the module tree" }
  - { name: container, desc: "Modules that hold and combine other sound generators" }
---
::
```

### Props

| Prop | Type | Required | Description |
|------|------|----------|-------------|
| `tags` | `Array<Tag>` | Yes | Category tag definitions |

```typescript
interface Tag {
  name: string;   // Category key from moduleList.json (e.g. "routing")
  desc: string;   // Category description from moduleList.json
}
```

### Category Colour Map

Each category has a dedicated colour. Define these as CSS custom properties alongside the existing theme constants:

```css
:root {
  --hise-cat-oscillator:       #f59e0b;
  --hise-cat-sample_playback:  #d97706;
  --hise-cat-container:        #64748b;
  --hise-cat-sequencing:       #a78bfa;
  --hise-cat-note_processing:  #8b5cf6;
  --hise-cat-dynamics:         #ef4444;
  --hise-cat-filter:           #22d3ee;
  --hise-cat-delay:            #14b8a6;
  --hise-cat-reverb:           #38bdf8;
  --hise-cat-mixing:           #a1a1aa;
  --hise-cat-input:            #4ade80;
  --hise-cat-generator:        #34d399;
  --hise-cat-routing:          #60a5fa;
  --hise-cat-utility:          #737373;
  --hise-cat-custom:           #f472b6;

  /* Scripting API domain groups */
  --hise-cat-module-tree:      #60a5fa;
  --hise-cat-ui:               #f59e0b;
  --hise-cat-data:             #14b8a6;
  --hise-cat-event:            #a78bfa;
  --hise-cat-scriptnode:        #f472b6;
  --hise-cat-services:         #4ade80;

  /* Scripting API roles (utility already defined above) */
  --hise-cat-handle:           #38bdf8;
  --hise-cat-factory:          #34d399;
  --hise-cat-component:        #d97706;
  --hise-cat-service:          #ef4444;
  --hise-cat-processor:        #22d3ee;
  /* --hise-cat-event: reuses the event group colour above */
  /* --hise-cat-container: reuses the module container colour above */
}
```

The component resolves the colour for each tag from `--hise-cat-{name}`. If the variable is not defined, fall back to `var(--hise-text-muted)`.

### Display Name Mapping

Category keys use snake_case internally. The component converts them to title-case display names for rendering:

| Key | Display Name |
|-----|-------------|
| `oscillator` | Oscillator |
| `sample_playback` | Sample Playback |
| `container` | Container |
| `sequencing` | Sequencing |
| `note_processing` | Note Processing |
| `dynamics` | Dynamics |
| `filter` | Filter |
| `delay` | Delay |
| `reverb` | Reverb |
| `mixing` | Mixing |
| `input` | Input |
| `generator` | Generator |
| `routing` | Routing |
| `utility` | Utility |
| `custom` | Custom |
| `module-tree` | Module Tree |
| `ui` | UI |
| `data` | Data |
| `event` | Event |
| `scriptnode` | Scriptnode |
| `services` | Services |
| `handle` | Handle |
| `factory` | Factory |
| `component` | Component |
| `service` | Service |
| `processor` | Processor |

Conversion rule: split on `_` or `-`, capitalise each word, join with space. Exception: `ui` renders as `UI`.

### Link Destination

Each tag links to the categories index page with a kebab-case anchor derived from the display name. The base URL depends on the page's domain:

| Domain | Base URL |
|--------|----------|
| Audio Modules | `/v2/reference/audio-modules/categories` |
| Scripting API | `/v2/scripting-api/` |

The component should detect which domain it is in from the current route prefix and use the appropriate base URL.

```
{base-url}#{kebab-display-name}
```

Conversion rule: take the display name (split `name` on `_` or `-`, capitalise each word, join with space), then lowercase and replace spaces with hyphens. Examples:

| `name` | Display Name | Anchor |
|--------|-------------|--------|
| `routing` | Routing | `#routing` |
| `sample_playback` | Sample Playback | `#sample-playback` |
| `note_processing` | Note Processing | `#note-processing` |
| `module-tree` | Module Tree | `#module-tree` |
| `ui` | UI | `#ui` |

### Rendered Structure

```html
<div class="ct-tags">
  <!-- Repeat per tag -->
  <a class="ct-tag" href="/v2/reference/audio-modules/categories#{kebab-display-name}"
     style="--ct-color: var(--hise-cat-{name})">
    <span class="ct-dot"></span>
    {Display Name}
  </a>
</div>

<!-- Shared tooltip (one per component instance) -->
<div class="ct-tooltip">{desc}</div>
```

### Tooltip Behaviour

- **Trigger:** `mouseenter` on any `.ct-tag` element.
- **Content:** The `desc` string from the tag entry.
- **Positioning:** Fixed position, appears below the tag. Clamped to viewport edges (max 340px width).
- **Dismiss:** `mouseleave` from the `.ct-tag` hides the tooltip.
- **No click blocking.** The tooltip is `pointer-events: none`; the tag link remains clickable at all times.

### Styles

```css
.ct-tags {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin: 8px 0 16px 0;
}

.ct-tag {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 3px 10px;
  border-radius: 12px;
  border: 1px solid color-mix(in srgb, var(--ct-color) 40%, transparent);
  background: color-mix(in srgb, var(--ct-color) 10%, transparent);
  color: var(--ct-color);
  font-family: var(--hise-font-body);
  font-size: 12px;
  font-weight: 500;
  text-decoration: none;
  cursor: pointer;
  transition: background 0.15s ease, border-color 0.15s ease;
}
.ct-tag:hover {
  background: color-mix(in srgb, var(--ct-color) 20%, transparent);
  border-color: color-mix(in srgb, var(--ct-color) 60%, transparent);
}

.ct-dot {
  width: 7px;
  height: 7px;
  border-radius: 50%;
  background: var(--ct-color);
}

.ct-tooltip {
  position: fixed;
  display: none;
  background: #1e293b;
  border: 1px solid #334155;
  border-radius: 6px;
  padding: 8px 12px;
  max-width: 340px;
  font-family: var(--hise-font-body);
  font-size: 12px;
  line-height: 1.5;
  color: var(--hise-text);
  pointer-events: none;
  z-index: 100;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.4);
}
```

---

## Component Registration

Register all four components as global Nuxt Content MDC components in `components/content/`:

```
components/
  content/
    CategoryTags.vue
    SignalPath.vue
    ParameterTable.vue
    ModulationTable.vue
```

Nuxt Content automatically maps `::category-tags` to `CategoryTags.vue`, `::signal-path` to `SignalPath.vue`, `::parameter-table` to `ParameterTable.vue`, and `::modulation-table` to `ModulationTable.vue` (kebab-case to PascalCase).

---

## Accessibility

- All highlighted spans use `cursor: help` to indicate interactivity.
- Tooltip text is readable at the tooltip's font size (13px body, 11px meta).
- Table rows have hover states for scanability.
- The pseudo-code label makes it clear this is not executable code.
- Colour is not the only differentiator - the legend provides text labels, and tooltips show the category in the name line.
- Category tags use both a coloured dot and text label. Tags are rendered as `<a>` elements for keyboard accessibility and screen reader link semantics.

---

## Edge Cases

- **Empty glossary category:** Omit that category's legend dot. The highlighting engine skips empty categories.
- **No modulation chains:** The page omits the `::modulation-table` block entirely (handled at authoring time, not render time).
- **Long pseudo-code:** The code block scrolls horizontally. No line wrapping.
- **Glossary key appears in a comment:** The highlighting engine skips full-line comments but will highlight keys in inline comments after `//`. This is acceptable - inline comments are rare in the pseudo-code style.
- **Overlapping glossary keys:** Longest-first matching prevents partial matches. If two keys are substrings of each other (e.g. `Attack` and `AttackLevel`), the longer one matches first and is wrapped in a span, preventing the shorter one from matching inside it.
- **Single category tag:** The flex container renders one tag; no special handling needed.
- **Unknown category key:** If `--hise-cat-{name}` is not defined, the tag falls back to `var(--hise-text-muted)` for colour.
