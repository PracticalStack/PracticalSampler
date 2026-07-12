# Phase 2: Project Example Extraction

Phase 2 provides real-world usage examples and project context extracted from actual HISE projects. It has two stages:

1. **External extraction** (private repo) - an extraction agent reads Phase 1 output, searches real project codebases, anonymizes examples, and writes raw output to `enrichment/phase2/ClassName/`.
2. **Test metadata enrichment** (this pipeline) - the pipeline agent adds slugs, test metadata, setup scripts, and test-only markers to the extracted examples, then validates them.

The extraction pipeline and project source code are maintained outside this repo for proprietary reasons. **Do not fabricate Phase 2 content.** Hallucinated "project context" passes as grounded evidence when it isn't - the `"source": "project"` tag in the JSON means "extracted from a real codebase," not "plausible-sounding."

---

## Pipeline Gate

After Phase 1 completes, check whether `enrichment/phase2/ClassName/` exists for **every class** in the current batch.

- **All directories exist** -> proceed to test metadata enrichment (below), then Phase 3 and Phase 4a.
- **Any directory missing** -> **STOP.** Report which classes need Phase 2 extraction and wait. Do not proceed with any class in the batch until all directories exist.

Directory existence is the signal. If the directory exists (even empty), extraction is complete for that class. The user will launch the extraction agent separately and tell you when to resume.

---

## Test Metadata Enrichment

Once Phase 2 files exist, the pipeline agent enriches examples with test metadata so they can be validated. This is a **combined pass covering both Phase 1 and Phase 2 examples** for the target class -- not just Phase 2 files. Phase 1 examples in `phase1/ClassName/methods.md` and Phase 2 examples in `phase2/ClassName/*.md` all receive slugs, testMetadata blocks, and setup/test-only markers in one go, then are validated together.

Follow `style-guide/scripting-api/test-metadata.md` for the full schema, verification types, setup patterns, and CLI reference.

### Per-method workflow

For each method file in `enrichment/phase2/ClassName/`:

1. **Skip class-level Readme.md** - Project Context and Common Mistakes do not need test metadata.

2. **Add slugs to code fences.** Change bare ```` ```javascript ```` to ```` ```javascript:slug-name ````. Slugs are kebab-case, unique per method within this phase. Derive from the example's title or purpose (e.g., `track-velocity-per-note`, `channel-routing-table`).

3. **Analyze each example for testability.** Ask: can this run standalone in HISE's onInit without external resources?
   - **Testable:** Self-contained, deterministic, no external files/MIDI/hardware.
   - **Not testable:** Requires audio files, DAW interaction, MIDI controllers, or project-specific resources. Mark `testable: false` with a `skipReason`.

4. **Complete incomplete examples.** Extraction output sometimes contains fragments (undefined variables, missing `const var` declarations, referencing objects that don't exist). Make the example runnable:
   - Add inline setup blocks (`// --- setup ---` / `// --- end setup ---`) for UI components or modules the example references.
   - Declare missing variables with realistic values.
   - If the example cannot be completed without destroying its purpose, mark `testable: false`.

5. **Add test-only blocks** where needed. If the example registers a callback that needs a programmatic trigger, add `// --- test-only ---` / `// --- end test-only ---` markers with the trigger code. See `test_metadata.md` for the trigger table (transport, broadcasters, control callbacks, cable callbacks, etc.).

6. **Write testMetadata blocks.** Add a ```` ```json:testMetadata:slug-name ```` block after each example with:
   - `testable`: boolean
   - `skipReason`: string (when not testable)
   - `verifyScript`: verification steps (log-output, REPL, or expect-error)

7. **Run validation** (covers both Phase 1 and Phase 2 examples):
    ```bash
    python snippet_validator.py --validate --source all --class ClassName --launch
    ```
    Fix failing examples and re-validate until all testable examples pass.

### What NOT to change

- Do not rewrite the example code beyond what's needed for testability. The examples were extracted from real projects - preserve their patterns, variable names, and structure.
- Do not add examples. Phase 2 content comes from the extraction agent only.
- Do not modify the class-level Readme.md (Project Context, Common Mistakes).

---

## Source Directory

```
enrichment/phase2/ClassName/Readme.md        # Class-level project context
enrichment/phase2/ClassName/methodName.md    # Method examples + pitfalls
```

One file per method. Only methods that have project examples need files here - this directory is sparse.

---

## Method File Format

Method files contain examples, optional pitfalls, and optional cross-references. They do NOT contain structured override fields (signature, parameters, description, callScope).

```markdown
## methodName

**Examples:**

```javascript:slug-name
// Title: What this example demonstrates
// Context: When/why you'd use this pattern
const var obj = Engine.createSomething();
obj.methodName(args);
```
```json:testMetadata:slug-name
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "obj.value", "value": 42}
}
```

**Pitfalls:**
- A real-world gotcha discovered in project analysis.

**Cross References:**
- `$API.OtherClass.relatedMethod$`
```

All sections are optional except `**Examples:**`.

---

## Class-Level Readme Format

`enrichment/phase2/ClassName/Readme.md` provides project-derived context. It populates a separate `projectContext` field in the JSON - it does not override Phase 1's `brief`, `purpose`, or `details`.

```markdown
# ClassName -- Project Context

## Project Context

### Real-World Use Cases
- **Use case name**: What kind of plugin builds with this class,
  how it fits into the architecture, and why this class was chosen.

### Complexity Tiers
1. **Tier name** (most common): Which methods are needed. Brief description.
2. **Next tier**: Additional methods. Brief description.

### Practical Defaults
- Most common configuration choice, backed by project adoption data.

### Integration Patterns
- `ClassName.method()` -> `OtherClass.method()` - when this pattern is used.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Project-discovered mistake | Correct approach | Why |
```

All four sub-sections within `## Project Context` are optional.

- **Real-World Use Cases**: Describe use case archetypes without identifying specific projects.
- **Complexity Tiers**: Numbered 1-N, simplest to most complex. Names specific methods per tier.
- **Practical Defaults**: State as direct expert recommendations.
- **Integration Patterns**: Method-to-method connections across classes, observed in project code.

---

## Merge Rules

Phase 2 contributes examples, pitfalls, cross-references, project context, and common mistakes. It does not override structural method fields (signature, parameters, description, callScope).

### Method-level

| Field | Merge Behavior | Tag |
|-------|---------------|-----|
| `examples` | **Last-writer-wins** (replaces Phase 1 array) | `"project"` |
| `pitfalls` | **Merged union** (added to Phase 1 pitfalls) | `"project"` |
| `crossReferences` | **Merged union** (deduplicated) | -- |

### Class-level

| Field | Target in JSON | Merge Behavior | Tag |
|-------|---------------|---------------|-----|
| `## Project Context` | `description.projectContext` | **Additive** (Phase 2-exclusive field) | `"project"` |
| `## Common Mistakes` | `commonMistakes[]` | **Merged union** (added to Phase 1 mistakes) | `"project"` |

The `projectContext` field is Phase 2-exclusive. It does not exist in Phase 1 and is not overridden by later phases. It serves as input for Phase 4a when writing user-facing documentation.

---

## Test Metadata Reference

See `style-guide/scripting-api/test-metadata.md` for:
- Full testMetadata schema (testable, skipReason, setupScript, verifyScript)
- Verification types (log-output, REPL, expect-error)
- Setup patterns (Builder API, saveInPreset, callback triggering)
- CLI commands (coverage, extract, validate, add-metadata)
