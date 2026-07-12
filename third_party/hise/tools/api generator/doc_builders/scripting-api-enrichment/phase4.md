# Phase 4a: User-Facing Documentation Authoring - Agent Instructions

Phase 4a transforms the raw C++ analysis from Phases 1-3 into user-facing documentation for HISEScript developers. The agent reads the complete merged `api_reference.json` for a single class and produces `userDocs` content for the class overview and each method. Phase 4a also renders SVG diagrams for methods/classes that have a `diagram` specification.

> **Note:** This file covers Phase 4a (human-facing docs). For Phase 4b (LLM C++ reference), see `phase4b.md`.

**Audience:** HISEScript developers who have no knowledge of C++ internals. They want to know what a method does, how to use it, and what to watch out for.

**ASCII-only rule:** All output files must use ASCII characters only. No em-dashes (use a regular dash or rewrite the sentence), no curly quotes.

**British English:** Use British spelling throughout all output files (behaviour, normalised, serialised, specialised, colour, etc.). See `style-guide/scripting-api/userdocs-style.md` for the full word list.

**Pitfalls and common mistakes are authored here.** Phase 1 and Phase 2 produce structured pitfall and common mistake data in the JSON. Phase 4a is responsible for curating and integrating this content directly into the `.md` files (as blockquote warnings and a Common Mistakes section). The merge/preview pipeline does not inject them mechanically - what you write is what appears on the page.

---

## Source Material

The agent reads from `enrichment/output/api_reference.json`, specifically the entry for the target class. The following fields are the primary source material:

### Class level
- `description.brief` -- one-line summary
- `description.purpose` -- technical summary (2-5 sentences)
- `description.details` -- full technical reference (may reference C++ internals)
- `description.projectContext` -- real-world usage context from analyzed HISE projects (Phase 2). Contains structured sub-sections: Real-World Use Cases (concrete examples from specific projects), Complexity Tiers (progressive method groups from beginner to advanced), Practical Defaults (most common configuration choices with evidence), and Integration Patterns (method-to-method connections with other HISE systems). Use this to ground the user-facing overview in practical reality -- it provides the "how it's actually used" perspective that C++ analysis cannot. When present, prefer project-derived use cases over generic descriptions.
- `description.codeExample` -- usage example
- `description.obtainedVia` - how the object is obtained in HISEScript
- `commonMistakes` - common mistakes table (Phase 1 auto + Phase 2 project-tagged entries; curate into a `## Common Mistakes` section in the Readme.md)
- `constants` -- named constants defined on the class (status codes, mode enums, location types). See "Constants Integration" below for how to present these.

### Method level
- `description` - technical description (may reference C++ internals)
- `signature` - method signature with types
- `parameters` - parameter table with types and descriptions
- `pitfalls` - non-obvious behaviors (Phase 1/2 structured data; curate into userDocs as `> **Warning:**` blockquotes)
- `examples` - code examples
- `crossReferences` - related methods
- `diagram` - diagram specification (if present): `type` + `description`

### Injected Diary Notes

When diary notes exist for the class or method, they are injected into your prompt as additional context. These are conversational notes with integration patterns, design rationale, and domain insights. They describe purpose and use cases from a user perspective - prefer their framing over the C++ analysis in the JSON when they cover the same topic. Extract unique insights (patterns, conventions, workflows), strip conversational filler ("in order to", "just", "also"), and incorporate into your userDocs in tight technical style. If a diary note exceeds 500 lines, note this in your decision log.

---

## Output Directories

```
enrichment/phase4/auto/ClassName/       # LLM-generated userDocs + auto SVGs (agent writes here)
enrichment/phase4/manual/ClassName/     # Human-edited overrides + manual SVGs (never touched by agent)
```

The agent ONLY writes to `phase4/auto/`. Never read or modify files in `phase4/manual/`.

---

## Output File Format

### Class-level: `phase4/auto/ClassName/Readme.md`

```markdown
# ClassName

[Opening sentence: what the class is and what it's for.]

[If the class has 3+ capabilities, modes, or target types, break them
into a numbered or bulleted list rather than inlining them in prose.]

[1-2 sentences of purpose-driven prose: when you'd use this class,
what problems it solves. Describe capabilities by purpose, not by
listing method names.]

[Optional: fenced code block showing how to create/obtain the object,
if the expression is non-trivial.]

[Optional: table for modes, sync options, or value descriptions.]

[Optional: ![diagram](filename.svg) embedding.]

> [Class-wide behavioural notes in a blockquote: build restrictions,
> thread safety, global-vs-local scope.]

## Common Mistakes

- **Wrong:** [code or description of the mistake]
  **Right:** [code or description of the correct approach]
  *[1-2 sentence explanation of why the wrong version fails.]*

[Repeat for each curated common mistake.]
```

The `# ClassName` heading is required. The overview content below it should be concise and scannable - no subheadings in the overview prose, but markdown tables, bulleted/numbered lists, fenced code blocks, and blockquotes are all encouraged where they improve readability (see `userdocs_style.md` for the full rules on each).

**Key structural rules:**
- **Break inline enumerations** of 3+ items into bulleted or numbered lists.
- **Pull class-wide behavioural notes** (build restrictions, thread safety, global/instance scope) into `>` blockquotes at the end of the overview, before Common Mistakes.
- **No method catalogues.** Describe what the class does by purpose, not by listing method names. Referencing 1-2 methods to illustrate a workflow is fine; listing 3+ as a catalogue is not.
- **Fenced code blocks** for non-trivial construction patterns (1-3 lines).
- **British English** throughout (behaviour, normalised, serialised, specialised, etc.).

Target a length between the Phase 1 `purpose` and `details` - substantial enough to orient a scripter, but with all C++ internals stripped. Scale the tone to the class complexity: simple utility classes should be approachable and purpose-driven; complex integration classes can use more technical language.

The `## Common Mistakes` section is optional but recommended for classes with non-trivial usage patterns. Curate from the `commonMistakes` array in the JSON - you have editorial discretion to omit entries that are too obvious, too niche, or redundant with the overview prose. The format must be exactly as shown above (the preview pipeline renders it from the markdown). Note: the preview pipeline strips the Common Mistakes section at render time - it is retained in the JSON for LLM consumers but does not appear on the HTML page.

### Method-level: `phase4/auto/ClassName/methodName.md`

```
[1-3 sentences describing what this method does and how to use it.
No heading required - the method name is inferred from the filename.]

> [!Warning:Concise problem title] non-obvious behavioral gotcha curated from the pitfalls array
```

Bare prose, no heading, no structured fields. Optionally followed by titled warning blockquotes for important pitfalls (see `style-guide/canonical-links.md` for the format). See "Pitfall Integration" below for curation guidance.

---

## Authoring Guidelines

See `style-guide/general.md` and `style-guide/scripting-api/userdocs-style.md` for the complete prose writing rules, including what to include, what to strip (C++ internals, preprocessor guards, implementation details), tone, length, class-level deduplication, formatting conventions, and reference examples.

---

## Pitfall Integration

Phase 1 and Phase 2 produce structured `pitfalls` data for methods. These remain in the JSON for LLM/MCP consumers. Phase 4a's job is to curate which pitfalls appear on the docs page and how they are phrased.

### Curation rules

1. **Check each method's `pitfalls` array** before writing its `.md` file.
2. **Integrate important pitfalls** as `> [!Warning:Title] ...` blockquotes at the end of the method's prose (see `style-guide/canonical-links.md` for the full format).
3. **Omit pitfalls that are:**
   - Too obvious from the signature (e.g., "throws an error if index is out of bounds")
   - Already caught by the API with an error message (the API handles it; no need to warn the reader)
   - Redundant with the prose you just wrote (do not restate the same fact in both prose and a warning)
   - Too niche or edge-case for the docs page (they stay in the JSON for power users via MCP)
4. **One warning per method is typical; two is the practical maximum.** More than two warnings makes the method feel like a minefield. If you have three important pitfalls, weave one into the prose naturally and use blockquotes for the other two.
5. **Do not duplicate between prose and warning.** Pick one location for each fact. If the prose already covers it, skip the blockquote.

### Format

```markdown
Registers a callback that fires on each musical beat. The callback receives
the beat index and a boolean for whether this is the first beat of a new bar.

> [!Warning:No immediate callback on registration] Does not fire immediately upon registration (unlike `setOnTempoChange` and `setOnTransportChange`). The first callback arrives at the next beat boundary.
```

The `> [!Warning:Title]` format renders as a titled callout on the docs page. The title should be 3-8 words, action-oriented. See `style-guide/canonical-links.md` for the full specification.

---

## Common Mistakes Integration

The `commonMistakes` array in the JSON contains wrong/right/explanation entries from Phase 1 (auto-detected) and Phase 2 (project-derived). Phase 4a curates these into a `## Common Mistakes` section in the class-level `Readme.md`. Each entry has a title, wrong pattern, right pattern, and explanation - see `style-guide/canonical-links.md` for the exact format.

### Curation rules

1. **Review the full `commonMistakes` array** including project-tagged entries.
2. **Keep entries that represent genuine, non-obvious traps.** A good common mistake is one where the code looks reasonable but silently does the wrong thing.
3. **Omit entries that are:**
   - Redundant with the overview prose (if the overview already explains the correct approach clearly, a "don't do the opposite" entry adds little)
   - Caught by the API with a clear error message (the scripter will see the error; no need to pre-warn)
   - Too specific to a single edge case
4. **Reword entries freely.** The Phase 1/2 text is raw analysis; your version should read naturally as user-facing guidance.
5. **Typical count: 2-5 entries.** Fewer for simple classes, more for classes with complex setup requirements.

---

## Constants Integration

Check the `constants` field in the JSON. When constants exist, integrate them into the overview prose where they are contextually relevant. Do not create a standalone "Constants Reference" section - weave them into the narrative.

### Presentation rules

- **Constants required to use a core method** (e.g. FileSystem's SpecialLocations for `getFolder()`, TransportHandler's SyncModes for `setSyncMode()`) - present in a table with behavioural descriptions in the class overview, at the point where the reader needs them.
- **Constants used in callback responses** (e.g. Server status codes) - present in a table near the prose that explains how to handle those responses.
- **Large constant sets** (100+ entries, e.g. Colours) - summarise the set and call out notable entries or gotchas. Do not reproduce the full list.

### Table format

Use a markdown table with at minimum Name and Value columns, plus a Description column when the meaning is not self-evident from the name. Prefix constant names with the class name (e.g. `Server.StatusOK`, not just `StatusOK`).

### Typical counts

| Constant count | Approach |
|----------------|----------|
| 1-8 | Table in the overview prose |
| 8-20 | Table, possibly grouped by category |
| 100+ | Summarise the set, call out notable entries |

---

## Editorial Self-Review

After writing all `.md` files (Readme + methods), re-read them together as a single page. Apply these edits directly to the source files:

1. **Consolidate repeated cross-method facts.** If 3+ methods state the same fact (e.g., "this is a global operation"), move it to the Readme and remove from individual methods. Keep it on a specific method only if it is genuinely surprising there.
2. **Resolve prose/warning overlap.** If a method's prose restates what its warning blockquote says, cut the weaker version.
3. **Remove filler.** "currently", "as its argument", "this specific", unnecessary "will".
4. **No em-dashes.** Use a regular dash or rewrite the sentence.
5. **Merge redundant sentences** within a single method's prose.
6. **Do not trim diagram-supporting prose.** Bullet lists, numbered enumerations, and short paragraphs that introduce or follow a diagram are *complementary*, not redundant. The diagram shows the visual shape (connections, topology, timeline); the prose explains details, names all items, and provides context the diagram intentionally omits for visual clarity. Never remove prose because "the diagram already shows this" -- they serve different modalities.

This step catches redundancy patterns that are only visible when all methods are read together. It is the final step before running the mechanical merge + preview.

---

## Decision Logging

Create a decision log in `enrichment/output/decisions/{ClassName}_phase4a.md` documenting **non-obvious decisions** made during authoring.

**What to Document (Non-Obvious Only):**

Skip obvious cases. Document judgment calls:

**Diary Notes:**
- What insights you incorporated from injected diary notes
- What you omitted (redundant, too conversational, not valuable)

**Diagram Decisions:**
- Why you rendered a diagram (complex topology, timing critical)
- Why you cut a diagram (redundant with prose/table, simple concept)

**Pitfall Integration:**
- When you merged multiple pitfalls into one warning (deduplication)
- When you omitted a pitfall (redundant, not actionable)

**Style Consolidations:**
- When you moved repeated facts from methods to class overview
- When you resolved prose/warning duplication

**Format:**

Structure the decision log with **two sections**:

1. **Class-Level Decisions** - big-picture decisions affecting the entire class
2. **Method Decisions** - per-method decisions, grouped under `### {MethodName}` headings

**Only include methods with non-obvious decisions.** Skip methods where all choices were obvious.

```markdown
# Phase 4a Authoring Decisions - {ClassName}

Generated: {timestamp}

## Class-Level Decisions

- Incorporated OSC address pattern from diary notes (/ prefix for OSC addresses)
- Rendered 3 diagrams (cable-dispatch, callback-threading, registerCallback)
- Consolidated "global operation" statement (repeated on 3 methods) into class overview

## Method Decisions

### setSyncMode

- Merged Phase 1 pitfall #2 and Phase 2 common mistake into single warning.

### startInternalClock

- Diary note is 650 lines - skimmed for patterns, incorporated timestamp workflow.

(Methods with no non-obvious decisions are not listed)
```

**Keep it concise:** Only document judgment calls, not obvious choices.

---

## Workflow

### Per-class execution

The agent runs once per class. **Diagrams are rendered first**, then userDocs are written (this allows the `.md` files to embed or link to diagram SVGs), then the editorial self-review pass runs.

1. Read the merged `api_reference.json`
2. Extract the target class entry
3. **Validate examples** (pre-authoring quality gate):
   ```bash
   python snippet_validator.py --validate --source all --class ClassName
   ```
   Fix any failing examples before proceeding. Do not write user-facing prose about examples that fail validation.
4. **Review injected diary notes** (if present):
   - Read and extract unique insights not in the JSON (integration patterns, use cases, workflow sequences, domain conventions)
   - Prefer diary framing for purpose and use cases over the C++ analysis
   - Note for decision log: what you incorporated, what you omitted
 5. **Triage diagrams** (see Diagram Triage below):
    - Review all diagram descriptions from Phase 1 (class-level `diagrams[]` and method-level `diagram` fields)
    - Default is to render all diagrams; cut only when a diagram is truly trivial, duplicates another diagram, or maps perfectly to a compact table
    - Cut diagrams are simply not rendered (the text description remains in the JSON for LLM consumers)
6. **Render surviving SVG diagrams** (see Diagram SVG Rendering below):
   - For each class-level diagram that survived triage, render an SVG
   - For each method diagram that survived triage, render an SVG
   - Skip if a manual or auto SVG already exists
7. Write class-level `Readme.md`:
   - Check `phase4/manual/ClassName/Readme.md` - if present, skip
   - Check `phase4/auto/ClassName/Readme.md` - if present, skip
   - Write `Readme.md` with user-facing prose incorporating diary insights in technical style
   - Add a `## Common Mistakes` section (curated from `commonMistakes` array)
   - Embed class-level diagram SVGs as `![brief](filename.svg)` (see Embedding Diagrams below)
   - Write complementary prose around each diagram: a short introductory sentence before the embed, and a bullet list or numbered enumeration after it that connects the visual elements to the details. The diagram shows the shape; the prose explains what the reader is looking at. This is not redundancy -- they serve different modalities.
8. Write method-level `.md` files:
   - Check `phase4/manual/ClassName/methodName.md` - if present, skip
   - Check `phase4/auto/ClassName/methodName.md` - if present, skip
   - Write `methodName.md` with user-facing prose incorporating diary insights
   - Integrate important pitfalls from the method's `pitfalls` array as `> **Warning:**` blockquotes (see Pitfall Integration above)
   - If the method has a `diagram` field that survived triage, embed the SVG as `![brief](filename.svg)`
   - If the method has a `diagramRef` field pointing to a rendered class-level diagram, link to the anchor: `[See: brief](#diagram-id)`
   - If the method's diagram or diagramRef was cut during triage, do not reference it
9. **Document authoring decisions** (see Decision Logging below):
   - Create `enrichment/output/decisions/{ClassName}_phase4a.md` with non-obvious decisions made during authoring
10. **Editorial self-review** (see Editorial Self-Review above):
   - Re-read all `.md` files together as a single page
   - Consolidate repeated facts, resolve prose/warning overlap, remove filler
   - Edit the source `.md` files directly

### Session prompt

```
Follow tools/api generator/doc_builders/scripting-api-enrichment/phase4.md.
Run phase4 authoring for [ClassName].
```

### After authoring

```bash
python api_enrich.py merge
python api_enrich.py preview [ClassName]
```

Then review `[ClassName].html` for user-facing quality. If any method's `userDocs` needs manual adjustment:

1. Copy `phase4/auto/ClassName/methodName.md` to `phase4/manual/ClassName/methodName.md`
2. Edit the manual copy
3. Re-run merge + preview

---

## Reference Examples

See `style-guide/scripting-api/userdocs-style.md` for two completed reference examples (Console and ScriptedViewport) that illustrate the target style. Review both before authoring a new class.

---

## Diagram Triage

Phase 1 generates diagram descriptions generously -- every pattern that could benefit from visualization gets a description. Phase 4 triages these descriptions before rendering.

### Default: render

**Render all diagrams by default.** It is far cheaper for the reviewer to delete a diagram from the output than to request one be added after the fact (which requires re-running Phase 4a with specific instructions). The bias should be strongly toward rendering.

### Triage criteria

Cut a diagram only when one of these conditions is clearly met:

1. **The diagram is truly trivial.** A simple linear flow (A -> B -> C) with no branching, no fan-in/fan-out, and no threading distinction adds nothing over a sentence of prose. Cut it.

2. **The diagram duplicates another diagram in the same class.** If a method-level diagram shows the same topology as a class-level diagram with no additional detail, cut the method-level one and use a `diagramRef` instead.

3. **The information is already a compact table.** If the concept maps perfectly to 3-4 rows in a table (e.g. "three modes with different properties"), a table may serve better than a diagram. But if the concept involves connections, flow, or sequencing, the diagram wins even if a table could technically list the same facts.

When in doubt, render. The reviewer will cut what does not work.

### What happens to cut diagrams

Cut diagrams are simply not rendered as SVGs. The plain-text `description` field remains in `api_reference.json` and is still available to LLM consumers (MCP server, autocomplete) who cannot render SVGs anyway. No data is lost -- only the visual rendering is skipped.

### Triage output

Before rendering, briefly note the triage decision for each diagram in a comment block at the top of the class `Readme.md`:

```markdown
<!-- Diagram triage:
  - topology-overview: RENDER (complex fan-in/fan-out not expressible in prose)
  - setup-sequence: RENDER (replaces setup paragraph)
  - connection-states: CUT (3 states already clear in mode table)
-->
```

This makes triage decisions reviewable. The comment is stripped by the merge script and does not appear in final output.

---

## Diagram SVG Rendering

When a method or class has diagram data that survived triage, Phase 4 renders it as an SVG file. **Diagrams are rendered in step 4 (before userDocs writing)** so that the `.md` files can reference them.

### SVG file naming

- Class-level diagrams: `{type}_{id}.svg` (e.g. `state_viewport-modes.svg`)
- Method-level diagrams: `{type}_{methodName}.svg` (e.g. `timing_registerCallback.svg`)

### Rendering workflow

1. Check `phase4/manual/ClassName/{filename}.svg` -- if present, skip (hand-crafted)
2. Check `phase4/auto/ClassName/{filename}.svg` -- if present, skip (already rendered)
3. Generate an SVG diagram based on `diagram.type`, `diagram.description`, and the full method/class context
4. Write to `phase4/auto/ClassName/{filename}.svg`

### Embedding diagrams in userDocs

After rendering, the `.md` files reference diagrams using standard markdown:

**Class-level `Readme.md`:** Embed class diagrams inline at a natural position in the prose:
```markdown
![Table Mode Setup Sequence](sequence_table-setup.svg)
```

**Method-level `methodName.md`:** Two cases:
- Method has its own `diagram` field: embed directly with `![brief](filename.svg)`
- Method has a `diagramRef` field: link to the class-level diagram anchor instead of embedding:
  ```markdown
  [See: ScriptedViewport Operating Modes](#diagram-viewport-modes)
  ```
  The anchor format is `#diagram-{id}` where `{id}` is the diagram's `id` field.

A method has `diagram`, `diagramRef`, or neither. Never both.

### Manual override

To lock a diagram (prevent re-generation), copy it from `phase4/auto/` to `phase4/manual/`. Manual SVGs are never overwritten by the agent.

### Rendering guidelines

See `style-guide/scripting-api/diagrams.md` for the full rendering conventions (colors, layout, fonts, diagram type patterns).

---

## Quality Checklist

See `style-guide/scripting-api/userdocs-style.md` for the full pre-submission quality checklist.
