# Reference Page Format (Step 4)

**Purpose:** Author a complete module reference page as MDC markdown. The page combines interactive pseudo-code, parameter tables, modulation chain tables, and prose into a single document rendered by Nuxt.js with custom Vue components. This is the final deliverable of the enrichment pipeline.

**Input:** `module_enrichment/exploration/{ModuleId}.json` (graph) + `module_enrichment/exploration/{ModuleId}.md` (findings) + `module_enrichment/base/moduleList.json`
**Output:** `module_enrichment/pages/{ModuleId}.md`
**Design reference:** `module_enrichment/resources/reference/` (5 HTML prototypes)

---

## What This Format Is

The reference page is an **MDC markdown file** - standard markdown extended with Vue component blocks using Nuxt Content's `::component` syntax. The page is rendered by a Nuxt.js docs site with three custom components:

1. **`::signal-path`** - Interactive pseudo-code with glossary-based syntax highlighting and hover tooltips
2. **`::parameter-table`** - Grouped parameter display with ranges, defaults, and descriptions
3. **`::modulation-table`** - Modulation chain listing with scope and constrainer info

Regular markdown handles the rest: frontmatter metadata, headings, prose paragraphs, screenshots, and notes.

---

## Page Template

Every module reference page follows this structure:

```markdown
---
title: Module Display Name
moduleId: ModuleId
type: Effect|Modulator|SoundGenerator|MidiProcessor
subtype: MasterEffect|EnvelopeModulator|VoiceStartModulator|...
tags: [category tags from moduleList.json]
builderPath: b.Effects.Delay
screenshot: /images/v2/reference/audio-modules/moduleid.png
cpuProfile:
  baseline: low|medium|high|very_high
  polyphonic: true|false
  scalingFactors: []
seeAlso:
  - { id: ModuleId, type: alternative|scriptnode|ui_component|..., reason: "..." }
  # seeAlso in frontmatter is for machine consumption (MCP server, llmRef).
  # The rendered page uses a ::see-also MDC component in the body (see "See Also Section" below).
commonMistakes:
  - { wrong: "...", right: "...", explanation: "..." }
customEquivalent:
  approach: hisescript|scriptnode|snex
  moduleType: "..."
  complexity: trivial|simple|medium|complex
  description: "..."
---

::category-tags
---
tags:
  - { name: category_key, desc: "Category description from moduleList.json" }
---
::

![Module screenshot](/images/v2/reference/audio-modules/moduleid.png)

[1-2 paragraphs of overview prose. What the module does, when to use it,
key characteristics. Written for HISEScript developers - no C++ internals.]

## Signal Path

::signal-path
---
glossary:
  parameters:
    ParamName:
      desc: "What this parameter controls"
      range: "0 - 100%"
      default: "50%"
  functions:
    funcName:
      desc: "What this processing step does"
  modulations:
    ModChainName:
      desc: "What this modulation chain scales"
      scope: "per-voice"
---

` `` [pseudo-code language marker not needed]
// Module Name - brief description
// [input type] in -> [output type] out

callbackOrFlow() {
    // pseudo-code here
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

## Modulation Chains

::modulation-table
---
chains:
  - { name: ChainName, desc: "What it scales", scope: "per-voice", constrainer: "VoiceStartModulator" }
---
::

### Monophonic Behaviour          <-- thematic subsection

[Substantial behavioural detail that deserves its own heading.]

### Visualisation with AHDSRGraph   <-- thematic subsection

[FloatingTile, LAF callbacks, setup tips.]

**See also:** $MODULES.EquivalentModule$ -- description, $API.Effect.setBypassed$ -- scripting API link
```

### Section Rules

- **Frontmatter** is required. All structured metadata lives here (seeAlso, cpuProfile, commonMistakes, customEquivalent). The `title` field generates the page's `h1` heading automatically - do not add a `# heading` manually.
- **Category Tags** is required. Place `::category-tags` as the first content after the frontmatter, before the screenshot. Embed the tag name and description for each category from `moduleList.json`.
- **Overview prose** is required. 1-2 paragraphs.
- **Signal Path** is required for all modules except containers and custom-category modules.
- **Parameters** is required if the module has parameters.
- **Modulation Chains** is required if the module has modulation chains. Omit the section entirely if there are none.
- **Thematic subsections** (`###`) are placed after the Modulation Chains section. Each covers a distinct behavioural topic (e.g. "Monophonic Behaviour", "Playback Modes", "Visualisation with AHDSRGraph", "Scripting with Tables"). Do not use a catch-all `## Notes` section - see "Content Distribution" below.
- **See Also** is optional. Uses the `::see-also` MDC component in the body (not plain markdown). Only include entries that are closely related but non-obvious to someone unfamiliar with the HISE architecture. Do not force a minimum number of entries - omit the section entirely rather than padding with loosely related modules.

---

## Prose Authoring Rules

For general writing style, tone, spelling, and what to strip, see `style-guide/general.md`. For cross-reference links, warnings, and tips format, see `style-guide/canonical-links.md`.

### What to Include (Module-Specific)

- What the module does in practical terms
- How the signal flows through it (the pseudo-code covers this visually; the prose provides context)
- Parameter interactions and non-obvious behaviours
- When to use this module vs alternatives
- Practical limitations
- Scale to module complexity: simple modules (ChannelFilter) get brief prose, complex modules (AHDSR, WaveSynth) get fuller explanations

### Vestigial Parameters

Vestigial parameters (defined but non-functional) should be noted factually in the parameter table description: "This parameter has no effect." No separate mention elsewhere is needed.

---

## Pseudo-Code Authoring

### Purpose

The pseudo-code is a visual explanation of the module's internal signal path. It is NOT executable code and should not be mistaken for a real API. It exists to communicate the processing logic more concisely than prose.

The `::signal-path` component renders a subtle label: "Pseudo-code - hover highlighted terms for details" to make this clear.

### Deriving from Graph JSON

The graph JSON from Step 3 (`module_enrichment/exploration/{ModuleId}.json`) is the structured source for pseudo-code. The transformation:

1. **Walk the graph** from input nodes to output nodes following signal-flow edges.
2. **High-importance nodes** (>= 0.7) become code statements or expressions.
3. **Medium-importance nodes** (0.3-0.7) may become comments or be folded into adjacent statements.
4. **Low-importance nodes** (< 0.3) are typically omitted.
5. **Decision nodes** become `if/else` blocks.
6. **Feedback edges** become explicit loop patterns (write then read).
7. **Groups** may become logical sections separated by blank lines or comments.

### Style

- Use C/JS-style syntax: curly braces, dot notation, semicolons optional
- Use natural, descriptive names - not C++ method names
- Comments use `//` and describe what a section does, not how
- Indentation is 4 spaces
- Keep it short: 5-25 lines for most modules

### Callback Framing

Callback-style framing is **optional** - use it when it clarifies the execution model:

| Situation | Approach |
|-----------|----------|
| Audio processor with continuous input | `process(left, right) { ... }` |
| Envelope with distinct note-on/off phases | `onNoteOn() { ... }` / `onNoteOff() { ... }` |
| MIDI processor filtering events | `onMidiEvent(message) { ... }` |
| Simple modulator with no event trigger | Flat flow without a wrapper function |

Do not invent callback names that imply a real API. If wrapping in a function name feels forced, use a flat flow with a comment header instead.

### Glossary

The glossary is the key to the interactive highlighting. Every term that should be highlighted in the pseudo-code must appear in exactly one glossary category:

| Category | Colour | What to include | Tooltip shows |
|----------|--------|----------------|---------------|
| `parameters` | Blue (#4a9eff) | Module parameters from moduleList.json that appear in the pseudo-code | desc, range, default |
| `functions` | Orange (#f97316) | Non-obvious processing operations (not simple assignments or arithmetic) | desc |
| `modulations` | Green (#90FFB1) | Modulation chain names that appear as multipliers in the pseudo-code | desc, scope |

**Glossary key rules:**
- Keys must be valid identifiers (no spaces, no special characters)
- Keys must be unique across all three categories
- Use the exact parameter ID from moduleList.json for parameters
- Use the exact modulation chain name for modulations
- For functions, use the name as it appears in the pseudo-code

**What NOT to glossary-highlight:**
- Local variables (e.g. `wetL`, `attackTime`)
- Literal values
- Standard operators and keywords (`if`, `else`, `return`)
- Simple arithmetic operations

---

## Parameter Table

The `::parameter-table` component renders a grouped table with columns: Parameter, Description, Range, Default.

### Grouping

Group parameters by function, not by parameter index. Good group labels:
- "Envelope" (Attack, Hold, Decay, Sustain, Release)
- "Delay Time" (DelayTimeLeft, DelayTimeRight, TempoSync)
- "Feedback" (FeedbackLeft, FeedbackRight)
- "Curve Shape" (AttackCurve, DecayCurve)
- "Voice Mode" (Monophonic, Retrigger)

If a group of parameters is vestigial, label the group accordingly: "Filtering (Vestigial)".

### Data Source

Parameter metadata comes from `moduleList.json`. The agent resolves ranges, defaults, and descriptions. Descriptions should be rewritten for the user audience (strip C++ internals).

### Range Formatting

| Parameter type | Range format |
|---------------|-------------|
| Slider (ms) | `0 - 20000 ms` |
| Slider (%) | `0 - 100%` |
| Slider (Hz) | `20 - 20000 Hz` |
| Slider (dB) | `-100 - 0 dB` |
| Slider (0-1) | `0.0 - 1.0` |
| Button | `On / Off` |
| Discrete | `1 - 16` |
| ComboBox | List the items: `Sine, Triangle, Saw, Square, Noise` |

### Default Formatting

- Numeric defaults: use the unit (`20 ms`, `30%`, `0.5`)
- Button defaults: `On` or `Off`
- Dynamic defaults: `(dynamic)` if the default depends on runtime state

---

## Modulation Chains Table

The `::modulation-table` component renders a table with columns: Chain, Description, Scope, Constrainer.

Include all modulation chains from moduleList.json that are not disabled. For disabled chains, omit them or add a brief `hints` entry on the relevant parameter explaining why the chain is inactive.

---

## Content Distribution

Do NOT create a `## Notes` section. Every piece of information from the exploration findings should be triaged to the most specific location available. Use this decision table:

| Content type | Target location | Example |
|---|---|---|
| **Parameter warning/gotcha** | `hints` array on the parameter in `::parameter-table` | "If AttackLevel is below Sustain, Hold and Decay are skipped" -> hint on AttackLevel |
| **Parameter edge case** | `hints` array (type: `info`) on the parameter | "Exponential release cuts off at -80 dB" -> hint on LinearMode |
| **Module identity** (what it is/isn't, voice killer role) | Merge into overview prose | "The exclamation mark icon indicates voice killer role" -> intro paragraph |
| **CPU comparison / alternative guidance** | Merge into overview prose | "Lighter than AHDSR" -> intro paragraph |
| **Vestigial parameter** | Parameter table `desc` field | "This parameter has no effect." |
| **Already in commonMistakes** | Remove (no duplication) | |
| **Already in parameter/modulation table descriptions** | Remove (no duplication) | "Mod chains are evaluated once at note-on" -> already in modulation table desc |
| **Substantial behavioural detail** (modes, monophonic, scripting patterns) | Named `###` subsection after Modulation Chains | `### Playback Modes`, `### Monophonic Behaviour`, `### Scripting with Tables` |
| **FloatingTile / LAF callbacks** | Named `###` subsection | `### Visualisation with AHDSRGraph` |
| **Warning/Tip block** | Place within the most relevant `###` subsection, or inline after the relevant parameter table if no subsection exists | |
| **Bug/issue** | `module_enrichment/issues.md` only (never in user-facing docs) | |

### Parameter Hints Format

The `hints` array on a parameter adds contextual warnings, tips, or info directly below the parameter row:

```yaml
params:
  - name: Attack
    desc: "Time to sweep through the attack table"
    range: "1 - 20000 ms"
    default: "20 ms"
    hints:
      - type: warning
        text: "If a note is released before the attack completes, the table freezes at its current position."
      - type: info
        text: "Modulating below 1 ms can cause silence."
```

Hint types: `warning` (orange), `info` (blue), `tip` (green). Use sparingly - 0-2 hints per parameter is typical. Reserve warnings for gotchas that waste debugging time.

### Subsection Naming

Use descriptive `###` headings that name the topic, not generic labels:

- **Good:** `### Playback Modes`, `### Monophonic Behaviour`, `### Visualisation with AHDSRGraph`, `### Scripting with Tables`, `### One-Shot Mode`
- **Bad:** `### Notes`, `### Additional Information`, `### Other Details`

---

## See Also Section

The See Also section uses canonical `$DOMAIN$` link tokens that the `publish.py` script resolves to correct URLs and converts to `::see-also` MDC components. This decouples the authored content from the Nuxt.js URL structure.

### Format

```markdown
**See also:** $MODULES.SendFX$ -- shares the send bus routing architecture, $MODULES.GlobalEnvelopeModulator$ -- reads per-voice envelope values from this container
```

Cross-domain links work too:

```markdown
**See also:** $MODULES.SendFX$ -- related module, $API.Effect.setBypassed$ -- scripting API for bypass control
```

### Link token syntax

Use `$DOMAIN.Target$` tokens. See `style-guide/canonical-links.md` for the complete reference of all domains, resolution cascade, and validation. You do NOT need to know the exact URL structure - just use the canonical ID and the publish script resolves it with fuzzy matching.

### Deriving from frontmatter

The `**See also:**` line is derived from the frontmatter `seeAlso` array. For each entry `{ id: ModuleId, type: ..., reason: "..." }`, emit `$MODULES.{id}$ -- {reason}`.

### In-prose links

For links within the body text, use standard markdown links with `$DOMAIN$` tokens:

```markdown
This module works together with the [Global Envelope Modulator]($MODULES.GlobalEnvelopeModulator$) to provide...

Use [Effect.setBypassed]($API.Effect.setBypassed$) to toggle the effect from script.

### Relationship types to include

| Type | When to include |
|------|----------------|
| `scriptnode` | Module has a scriptnode equivalent |
| `ui_component` | Module has a dedicated FloatingTile |
| `alternative` | Similar module with different tradeoffs |
| `upgrade` | Newer module that replaces this one |
| `source` / `target` | Producer/consumer relationship (e.g. GlobalModulatorContainer -> GlobalVoiceStartModulator) |
| `companion` | Commonly used together |
| `disambiguation` | Easily confused with another module |

The relationship type is NOT included in the `desc` field - only the reason text. The type is stored in the frontmatter `seeAlso` array for machine consumption.

### Discovery

See `module-enrichment.md` section "seeAlso" for the full relationship taxonomy with reciprocals, primary sources, source/target examples, and known scriptnode equivalent pairs.

---

## Cross-Cutting Fields

These fields live in the frontmatter YAML and are consumed by the Nuxt.js site for sidebar display, search, and LLM serving.

### cpuProfile

Hardware-independent performance model. See `module-enrichment.md` section "cpuProfile" for tier definitions.

### commonMistakes

Array of `{wrong, right, explanation}` entries. Curate from exploration findings. Explanations must reference observable behaviour ("causes clicks", "wastes CPU"), never implementation details.

### customEquivalent

How to rebuild the module using custom modules. See `module-enrichment.md` section "customEquivalent".

### forumReferences

Array of 0-3 citation entries linking page claims back to forum discussions. Each entry:

| Field | Type | Description |
|-------|------|-------------|
| `id` | number | Citation number (1-based, matches `[N]` in prose) |
| `title` | string | Short title of the forum finding (from the insight's `title`) |
| `summary` | string | One-sentence summary of the finding (from the insight's `summary`) |
| `topic` | number | Forum topic ID (resolves to `https://forum.hise.audio/topic/{topic}`) |

Inline citations use the canonical link pattern: `[1]($FORUM_REF.2054$)` where the token contains the forum topic ID directly. The `$FORUM_REF.tid$` token is resolved by `publish.py` to `https://forum.hise.audio/topic/{tid}` — no frontmatter lookup needed. The Nuxt.js renderer auto-injects a `::forum-references` component from the frontmatter data — do not write this component in the MDC body.

**Selection criteria:** Pick the 2-3 most citation-worthy insights — non-obvious gotchas, confirmed bugs, and practical workarounds sourced from real user experience. Do not cite generic API facts or information already confirmed via C++ exploration.

Example:
```yaml
forumReferences:
  - id: 1
    title: "IR resampling causes gain increase at non-native sample rates"
    summary: "When the host sample rate differs from the IR's native sample rate, resampling introduces a gain increase (~6dB at 96kHz vs 44.1kHz). No built-in compensation."
    topic: 2054
```

With inline citation in prose:
```markdown
adjust WetGain manually if needed. [1]($FORUM_REF.2054$)
```

### llmRef

Pre-synthesised text blob for the MCP server. Written as a separate field in the frontmatter with fixed section order. See `module-enrichment.md` section "llmRef".

---

## Step 4: Author Reference Page

### Inputs

- `module_enrichment/exploration/{ModuleId}.json` - graph topology (nodes, edges, groups)
- `module_enrichment/exploration/{ModuleId}.md` - exploration findings (signal path, gap answers)
- `module_enrichment/base/moduleList.json` - parameter metadata

### Procedure

1. **Set up frontmatter and category tags.** Extract moduleId, prettyName, type, subtype, category, builderPath from moduleList.json. Set screenshot path. Add `::category-tags` block after the `# h1` heading with the module's categories and their descriptions from moduleList.json.

2. **Write overview prose.** 1-2 paragraphs explaining what the module does. Use the exploration markdown's Signal Path section as source. Strip all C++ internals per the translation table.

3. **Write pseudo-code.** Walk the graph JSON from input to output. Transform high-importance nodes into code statements. Use the callback framing table to choose an appropriate wrapper (or none).

4. **Build glossary.** For each parameter, function, and modulation chain referenced in the pseudo-code, add a glossary entry. Resolve parameter metadata (desc, range, default) from moduleList.json. Rewrite descriptions for the user audience.

5. **Build parameter table.** Group all parameters from moduleList.json semantically. Rewrite descriptions. Format ranges and defaults per the tables above.

6. **Build modulation chains table.** List all non-disabled modulation chains with descriptions, scope, and constrainer type.

7. **Distribute remaining content.** For each non-obvious behaviour from the exploration findings, triage it to the best location using the Content Distribution table below. Do NOT create a `## Notes` section - every piece of information should land in a specific, appropriate place.

8. **Write See Also.** Surface scriptnode equivalents, UI components, alternatives, and other relationships from the exploration.

9. **Build cross-cutting fields.** Write cpuProfile, commonMistakes, customEquivalent, llmRef into the frontmatter.

10. **Editorial review.** Re-read the entire page. Check for C++ leakage, redundancy between prose and pseudo-code, missing parameters, broken glossary references. Verify there is no `## Notes` section.

### Step 4 Gate Checklist

Before declaring the reference page complete:

- [ ] **Category tags:** `::category-tags` block is present after the `# h1` heading with correct tag names and descriptions from moduleList.json.
- [ ] **Parameter accountability:** Every parameter from moduleList.json appears in the parameter table. Parameters referenced in pseudo-code have glossary entries.
- [ ] **Modulation chain coverage:** Every non-disabled modulation chain is in the modulation table. Chains referenced in pseudo-code have glossary entries.
- [ ] **Pseudo-code completeness:** The main signal path from input to output is covered. All high-importance nodes from the graph JSON are represented.
- [ ] **Glossary consistency:** Every highlighted term in the pseudo-code has a glossary entry. No glossary entry is unused. No collisions between categories.
- [ ] **No C++ leakage:** No C++ class names, method names, type names, assertions, or source references in prose, pseudo-code, or table descriptions.
- [ ] **See Also relevance:** Entries are closely related and non-obvious. No padding with loosely related modules. Scriptnode equivalent included if one exists. UI component included if a FloatingTile exists. Empty list is acceptable.
- [ ] **Prose quality:** British English. No marketing language. No filler. Descriptions lead with what the module does, not how it is implemented.
- [ ] **No Notes section:** The page has no `## Notes` heading. All content is distributed to parameter hints, overview prose, named `###` subsections, or commonMistakes per the Content Distribution table.
- [ ] **Exploration incorporation:** All relevant findings from the exploration markdown are reflected in the page (prose, pseudo-code, parameter hints, subsections, or see-also).

---

## Category-Specific Patterns

Different module categories produce characteristic reference pages. These are guides, not templates.

### Envelope Modules

- Pseudo-code uses `onNoteOn()` / `onNoteOff()` framing
- Parameters grouped by stage (Envelope, Curve Shape, Voice Mode)
- Modulation chains table is usually substantial (one chain per stage)
- See Also typically includes scriptnode equivalent and AhdsrEnvelopePanel

### Audio Effects

- Pseudo-code uses `process(left, right)` framing
- Feedback loops shown explicitly in pseudo-code
- Dry/wet mix as final operation
- Vestigial filter parameters noted if present
- TempoSync noted in parameter groups

### Sound Generators

- Pseudo-code uses `process(left, right)` framing
- Multi-oscillator modules show parallel paths merging
- Waveform type tables may be needed (e.g. WaveSynth waveform icons)
- Modulation chains for pitch and gain

### MIDI Processors

- Pseudo-code uses `onMidiEvent(message)` framing or flat flow
- No audio path, no modulation output
- Parameters are typically simple (channel numbers, transpose amounts)
- Event handling details (ignore vs destroy) go in a `### Event Handling` subsection or overview prose

### Time-Variant Modulators

- Flat flow or event-driven framing depending on the trigger
- Simple parameter set
- May have no modulation chains
