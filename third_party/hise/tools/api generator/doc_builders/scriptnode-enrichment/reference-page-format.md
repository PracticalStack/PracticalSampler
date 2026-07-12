# Reference Page Format (Step 4)

**Purpose:** Author a complete node reference page as MDC markdown. The page combines interactive pseudo-code, parameter tables, and prose into a single document rendered by Nuxt.js with custom Vue components. This is the final deliverable of the enrichment pipeline.

**Input:** `scriptnode_enrichment/exploration/{factory}.{node}.json` (graph) + `scriptnode_enrichment/exploration/{factory}.{node}.md` (findings) + `scriptnode_enrichment/base/scriptnodeList.json`
**Output:** `scriptnode_enrichment/output/{factory}/{node}.md`
**Style guides:** `style-guide/general.md` (prose), `style-guide/canonical-links.md` (links, warnings, tips)
**Design reference:** `module_enrichment/pages/SimpleGain.md` (module page example for format reference)

---

## Page Template

Every node reference page follows this structure:

```markdown
---
title: Node Display Name
description: "One-sentence summary for page metadata and sub-header display."
factoryPath: factory.node
factory: factory
polyphonic: true|false
tags: [factory, additional-tags]
screenshot: /images/v2/reference/scriptnodes/{factory}/{node}.png
cpuProfile:
  baseline: negligible|low|medium|high|very_high
  polyphonic: true|false
  scalingFactors: []
seeAlso:
  - { id: "factory.other_node", type: alternative|companion|..., reason: "..." }
commonMistakes:
  - title: "3-8 word summary of the mistake pattern"
    wrong: "..."
    right: "..."
    explanation: "..."
llmRef: |
  factory.node_name

  [overview]

  Signal flow:
    [arrow notation]

  CPU: [tier], [voice context]

  Parameters:
    [grouped with notes]

  When to use:
    [guidance]

  See also:
    [entries]
---

![Node screenshot](/images/custom/scriptnode/{image}.png)

[1-2 paragraphs of overview prose. What the node does, when to use it,
key characteristics. No C++ internals.]

## Signal Path

::signal-path
---
glossary:
  parameters:
    ParamName:
      desc: "What this parameter controls"
      range: "min - max unit"
      default: "value"
  functions:
    funcName:
      desc: "What this processing step does"
---

` ``
// factory.node - brief description
// [input type] in -> [output type] out

process(input) {
    // pseudo-code
}
` ``

::

## Parameters

::parameter-table
---
groups:
  - label: Group Name
    params:
      - { name: ParamName, desc: "Description", range: "min - max unit", default: "value" }
---
::

## Notes

[Non-obvious behaviours, practical tips. Omit if nothing notable.]

**See also:** $SN.factory.other_node$ -- reason, $MODULES.ModuleId$ -- related module
```

### Section Rules

- **Frontmatter** is required. All structured metadata lives here.
- **Screenshot** is optional. Include only if an image exists in `resources/images/`.
- **Overview prose** is required. 1-2 paragraphs. Scale to complexity.
- **Signal Path** is required for all nodes except trivially simple ones where the parameter table alone is sufficient (e.g., `math.abs` - takes absolute value, nothing more to show).
- **Parameters** is required if the node has parameters.
- **Notes** is optional. Include only for non-obvious behaviours.
- **See Also** is optional. Do not force entries - omit rather than pad with loosely related nodes.

### Sections NOT Used (vs Module Pages)

Scriptnode node pages do NOT include:
- **`::category-tags`** - nodes use factory tags in the frontmatter, not rendered category badges
- **`::modulation-table`** - nodes do not have built-in modulation chains (modulation is external via cables)
- **`customEquivalent`** - nodes ARE the custom equivalent (they are the building blocks)
- **`builderPath`** - nodes are created via the scriptnode popup, not the Builder API

---

## Prose Authoring Rules

For general writing style, tone, spelling, and what to strip, see `style-guide/general.md`. For cross-reference links, warnings, and tips, see `style-guide/canonical-links.md`.

### What to Include

- What the node does in practical terms
- When to use this node vs alternatives
- Parameter interactions and non-obvious behaviours
- Practical limitations
- Scale to complexity: `math.add` gets 2-3 sentences; `core.granulator` gets 2-3 paragraphs

### What NOT to Include

- C++ class names, method names, template arguments
- Implementation mechanisms ("uses a PolyData member", "iterates the span")
- Bug descriptions or workarounds (these go to issues.md)
- Verbose parameter descriptions that repeat what the parameter table already shows

### Translation Table

| C++ exploration finding | User-facing prose |
|---|---|
| "Uses `PolyData<float, NV>` for per-voice state" | "Each voice maintains its own state" |
| "Calls `hmath::sin()` per sample" | "Applies a sine function to each sample" |
| "Inherits from `filter_base` with SVF topology" | "A state variable filter" |
| "Process iterates `d.toChannelData(ch)` for each channel" | "Processes each channel independently" |
| "Parameter smoothing via `sfloat`" | "Parameter changes are smoothed to prevent clicks" |
| "Compile-time channel count via `wrap::fix<2, T>`" | "Operates on stereo signals" |

---

## Pseudo-Code Authoring

### Purpose

The pseudo-code explains the node's processing logic visually. It is NOT executable code. The `::signal-path` component renders it with glossary-based syntax highlighting and hover tooltips.

### Deriving from Graph JSON

1. Walk the graph from input to output.
2. High-importance nodes (>= 0.7) become code statements.
3. Medium-importance nodes (0.3-0.7) may become comments.
4. Decision nodes become `if/else` blocks.

### Style

- C/JS-style syntax: dot notation, semicolons optional
- Natural, descriptive names - not C++ names
- Comments use `//`
- Indentation: 4 spaces
- **Keep it short.** Most nodes: 3-10 lines. Simple nodes: 1-3 lines. Complex nodes: up to 20 lines.

### Framing

| Node type | Approach |
|-----------|----------|
| Audio processor | `process(input) { ... }` or `process(left, right) { ... }` |
| Control node | `onValueChange(input) { ... }` or flat flow |
| Container | Describe dispatch pattern, not processing |
| Envelope | `onNoteOn() { ... }` / `onNoteOff() { ... }` |
| Analysis | `analyse(input) { ... }` or flat flow |

Do not invent callback names that imply a real API. If wrapping in a function feels forced, use a flat flow with a comment header.

### Simple Node Pseudo-Code

For `math.add`:

```
// math.add - adds a DC offset
// audio in -> audio out

process(input) {
    output = input + Value
}
```

For `control.pma` (parameter multiply-add):

```
// control.pma - scales and offsets a modulation signal
// control in -> control out

onValueChange(input) {
    output = input * Multiply + Add
}
```

### Glossary

Every highlighted term must appear in exactly one glossary category:

| Category | Colour | What to include |
|----------|--------|----------------|
| `parameters` | Blue (#4a9eff) | Parameters from scriptnodeList.json that appear in pseudo-code |
| `functions` | Orange (#f97316) | Non-obvious processing operations |

Note: no `modulations` category (unlike module pages) because scriptnode modulation is external.

**Glossary key rules:**
- Keys must match the exact parameter ID from scriptnodeList.json
- For functions, use the name as it appears in the pseudo-code
- Every highlighted term in pseudo-code needs a glossary entry
- No unused glossary entries

---

## Parameter Table

The `::parameter-table` component renders a grouped table with columns: Parameter, Description, Range, Default.

### Grouping

Group parameters by function:
- "Signal" (Value, input parameters)
- "Configuration" (Mode, scaling parameters)
- "Modulation" (parameters that control modulation behaviour)

If a node has only 1-2 parameters, a single unnamed group is fine.

### Range Formatting

| Parameter type | Range format |
|---------------|-------------|
| Slider (0-1) | `0.0 - 1.0` |
| Slider with units | `20 - 20000 Hz`, `0 - 500 ms` |
| Bipolar | `-1.0 - 1.0` |
| Button/toggle | `Off / On` |
| Discrete integer | `1 - 16` |
| Skewed | Note the skew in the description, not the range |
| Enum (from TextToValueConverter) | List the items if decodable, otherwise note "see Mode property" |

### Default Formatting

- Numeric: use the value from scriptnodeList.json
- Toggle: `Off` or `On`
- For 0.0 defaults on 0-1 ranges: just `0.0`

### Descriptions

- Lead with what the parameter controls, not how
- Note non-obvious behaviour (e.g., "Values above 1.0 boost the signal")
- For `UseUnnormalisedModulation` parameters: note that the modulation output sends the raw value, not normalised to 0-1
- Keep descriptions to 1-2 sentences

---

## See Also Section

Use canonical `$DOMAIN$` link tokens from `style-guide/canonical-links.md`.

### Format

```markdown
**See also:** $SN.math.mul$ -- multiplication equivalent, $MODULES.SimpleGain$ -- audio module with similar gain control
```

### Relationship Types

| Type | When to include |
|------|----------------|
| `alternative` | Similar node with different tradeoffs (svf vs biquad) |
| `companion` | Commonly used together (send + receive) |
| `disambiguation` | Easily confused (pma vs pma_unscaled) |
| `module` | Equivalent HISE audio module |
| `api` | Related scripting API class |

### Deriving from Frontmatter

The `**See also:**` line is derived from the frontmatter `seeAlso` array. For each entry, emit `$SN.{id}$ -- {reason}` (or `$MODULES.{id}$` / `$API.{id}$` for cross-domain).

---

## Cross-Cutting Frontmatter Fields

### cpuProfile

| Tier | Examples |
|------|---------|
| `negligible` | math.add, math.abs, control.change |
| `low` | core.gain, filters.one_pole, control.pma |
| `medium` | filters.svf, core.fix_delay, fx.bitcrush |
| `high` | filters.convolution, core.granulator, fx.reverb |
| `very_high` | analyse.fft (with large buffer), core.stretch_player |

### commonMistakes (0-3 per node)

Most simple nodes (math operators, basic control nodes) will have 0 entries. Include only when there is a genuine, non-obvious pitfall:

```yaml
commonMistakes:
  - wrong: "Using math.add with a large Value expecting volume boost"
    right: "math.add adds a DC offset. Use core.gain for volume control."
    explanation: "Adding a constant to an audio signal shifts its centre, introducing DC offset. For amplitude changes, multiply instead of add."
```

### llmRef

Fixed section order. Omit sections that are empty:

```
factory.node_name

[1-2 sentence overview]

Signal flow:
  [arrow notation, or "Control node - no audio processing"]

CPU: [tier], [polyphonic/monophonic]

Parameters:
  [grouped, practical notes, defaults]

When to use:
  [guidance]

Common mistakes:
  [if any]

See also:
  [type] factory.node -- reason
```

---

## Factory-Level Readme

Each factory also gets a `Readme.md` overview page at `output/{factory}/Readme.md`. This is written once when the first node in a factory is processed.

### Factory Readme Template

```markdown
---
title: {Factory Name} Nodes
factory: {factory}
---

[1-2 paragraphs explaining the factory's purpose and the common
characteristics of its nodes.]

## Nodes

| Node | Description |
|------|-------------|
| [$SN.factory.node1$]($SN.factory.node1$) | Brief description |
| [$SN.factory.node2$]($SN.factory.node2$) | Brief description |
```

The node table is populated as nodes are enriched. When enriching a new node in an already-started factory, add it to the existing Readme.

---

## Node Type-Specific Patterns

### Audio Processors

- Pseudo-code uses `process(input)` or `process(left, right)` framing
- Parameter table includes all audio-affecting parameters
- Note polyphonic behaviour if applicable

### Control Nodes

- Pseudo-code uses `onValueChange(input)` or flat flow
- Note whether output is normalised (0-1) or unnormalised (raw values)
- For multi-output nodes (xfader, branch_cable): show how outputs are distributed

### Container Nodes

- Pseudo-code shows dispatch pattern: serial (chain), parallel (split), branched (branch)
- The `::signal-path` glossary highlights the container's parameters (if any)
- Note bypass behaviour and any special processing context changes (block size, oversampling, MIDI filtering)

### Variant Nodes

- Each variant gets its own file (per the pipeline design)
- The description should note what varies and reference the base concept
- Parameter table shows only the variant-specific parameters
- Pseudo-code may be identical to the base variant with just the numeric difference noted

### Envelope Nodes

- Pseudo-code uses `onNoteOn()` / `onNoteOff()` framing
- Note modulation outputs (CV, Gate) and what they send
- Describe voice management integration

---

## Step 4 Gate Checklist

Before declaring the reference page complete:

- [ ] **Parameter accountability:** Every parameter from scriptnodeList.json appears in the parameter table. Parameters in pseudo-code have glossary entries.
- [ ] **Pseudo-code completeness:** The main signal path is covered. All high-importance graph nodes are represented.
- [ ] **Glossary consistency:** Every highlighted term has a glossary entry. No unused entries.
- [ ] **No C++ leakage:** No C++ class names, method names, template arguments, or source references in prose, pseudo-code, or table descriptions.
- [ ] **See Also relevance:** Entries are closely related and non-obvious. Empty is acceptable.
- [ ] **Prose quality:** British English. No marketing language. No filler. Leads with what the node does.
- [ ] **Exploration incorporation:** All relevant exploration findings are reflected in the page.
- [ ] **llmRef completeness:** All sections filled. Arrow notation matches pseudo-code.
- [ ] **cpuProfile present:** Baseline tier and polyphonic flag set.
- [ ] **Factory Readme updated:** Node is listed in the factory's Readme.md.
- [ ] **In-prose links wrapped:** All `$DOMAIN.Target$` tokens in body text are wrapped as markdown links `[label]($DOMAIN.Target$)`. Bare tokens in prose are not allowed - they resolve to unlinked URLs. **Exception:** `**See also:**` lines use bare tokens (`$DOMAIN.Target$ -- reason`) because `publish.py` converts them to `::see-also` MDC components.
- [ ] **Common mistake titles:** Every `commonMistakes` entry in frontmatter has a `title` field (3-8 words, describes the mistake pattern). The title is used as the heading in the rendered common mistakes component.
