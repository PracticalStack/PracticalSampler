# Scripting API Reference Enrichment Pipeline

**Purpose:** Produce a comprehensive `api_reference.json` that an MCP server uses to improve LLM-generated HiseScript code, serves as a complete technical developer reference, and provides concise C++ reference material for LLMs writing `HardcodedScriptProcessor` code.

**Output Location:** `tools/api generator/enrichment/output/api_reference.json`

**Sub-phase details:** See `scripting-api-enrichment/phase0.md` through `phase4.md` (Phase 4a) and `phase4b.md` for detailed per-phase instructions.

---

## Strategic Context: Shared API Surface

The HISE scripting API (`Synth`, `Message`, `Engine`, `TransportHandler`, `FixObjectFactory`, etc.) is not exclusive to HISEScript. The same API is available from C++ via `HardcodedScriptProcessor`, which inherits all the scripting callbacks (`onNoteOn`, `onNoteOff`, `onTimer`, etc.) and objects (`Synth.addNoteOn()`, `Message.ignoreEvent()`, `Engine.getHostBpm()`, etc.) with identical signatures and semantics. The existing `hise::Arpeggiator` class (`hi_scripting/scripting/hardcoded_modules/Arpeggiator.h/.cpp`) is a concrete example -- it's a C++ class that calls the same `Synth.addNoteOn()`, `Synth.noteOffDelayedByEventId()`, `Message.ignoreEvent()` methods that a HISEScript developer would use.

This means the Phase 1 C++ analysis -- which extracts method signatures, threading constraints, parameter semantics, edge cases, and the full call chain from source code -- serves **both** documentation paths:

- **Phase 4a** (human HISEScript docs): Prose descriptions, HISEScript examples, scripter-friendly language
- **Phase 4b** (LLM C++ reference): Structured, concise entries with source code references for LLMs writing either HISEScript or C++ `HardcodedScriptProcessor` code

Phase 4b requires **no new exploration**. It is a projection of existing Phase 1 data (the exploration `.md` files and `methods.md` output).

### The HISEScript / C++ Staircase

For LLM-driven workflows, HISEScript and C++ serve complementary roles:

| Complexity | Approach | Why |
|------------|----------|-----|
| **Snippet adaptation** | HISEScript | Task maps to 1-2 existing snippets. Fast compile cycle for iteration. LLM adapts proven patterns. |
| **Snippet combination** | HISEScript (with care) | Task requires combining patterns from multiple snippets. Higher risk, but still tractable. |
| **Beyond snippet coverage** | C++ via `HardcodedScriptProcessor` or `raw` namespace | The required primitives don't exist in HISEScript, or the algorithmic complexity exceeds what snippets can template. LLM uses the same scripting API from C++, plus full access to JUCE and the HISE source code. |

The threshold for escalation is not about task complexity -- it's about **whether the right scripting primitives exist**. When HISE added `TransportHandler` (grid callbacks, tempo sync), the HISEScript arpeggiator rewrite became cleaner than the original C++ `HardcodedScriptProcessor` version. The primitives define the ceiling, not the language.

### Snippets as Primitive Coverage Map

The snippet library (99 browsable snippets in the MCP server) serves a dual role:

1. **Implementation templates:** LLMs adapt snippets for tasks within the HISEScript sweet spot.
2. **Complexity threshold markers:** The snippet library defines the boundary of what HISEScript handles idiomatically. Tasks that map to existing snippets stay in HISEScript. Tasks that require inventing beyond the snippet library signal escalation to C++.

Gaps in snippet coverage for a given API domain (e.g., no snippet demonstrates `UnorderedStack.setIsEventStack()` or free-running `onTimer` for MIDI generation) are informative -- they indicate either candidates for new snippets or natural C++ territory.

### The `raw` Namespace

The `raw` namespace (`docs.hise.dev/cpp_api/raw/`) is the curated C++ entry point for the HISE codebase:

- `raw::Builder` -- constructs module architecture
- `raw::Reference` -- wraps a processor with parameter change callbacks
- `raw::UIConnection` -- bidirectional UI-processor binding
- `raw::Pool` -- loads embedded resources
- `raw::TaskAfterSuspension` -- safe async execution with audio thread suspension

For agentic C++ workflows, the `raw` namespace provides module architecture and UI wiring, while the scripting API (`Synth`, `Message`, `Engine`) handles callback logic. Both are documented by the enrichment pipeline.

---

## Pipeline Overview

```
Phase 0: batchCreate.bat -> xml/selection/*.xml -> enrichment/base/*.json
       |   (batch script + Python, 100% mechanical)
       |
Phase 1: C++ source analysis + example synthesis
       |   Step A1 (sub-agent) -> resources/explorations/ClassName.md (raw C++ extracts)
       |   Step A2 (sub-agent) -> ClassName/Readme.md + methods_todo.md (distilled)
       |   Step B  (main agent) -> ClassName/methods.md (per-method, fire-and-forget)
       |   Step C  (post-process) -> deduplicate, cross-ref, markdown -> JSON
       |
Phase 2: Pipeline gate (external project extraction) + test metadata enrichment
       |
Phase 3: Author's diary (code examples + cross-refs extracted mechanically)
       |
Phase 4a: User-facing documentation authoring (human HISEScript developers)
       |   Agent-driven -> phase4/auto/ClassName/*.md (LLM-generated userDocs)
       |   Human-edited -> phase4/manual/ClassName/*.md (overrides auto)
       |
Phase 4b: LLM C++ reference authoring (LLMs writing HISEScript or C++)
       |   Source: Phase 1 methods.md + explorations/*.md (no new exploration)
       |   Output -> phase4b/ClassName/methodName.md (structured LLM reference)
       |
python api_enrich.py merge -> output/api_reference.json
python api_enrich.py preview -> output/preview/ (plain markdown + HTML)
                             -> output/website/ (MDC post-processed via postprocess_md.py)
```

---

## Directory Structure

```
tools/api generator/
├── batchCreate.bat                           # existing, unchanged
├── xml/selection/                            # existing Doxygen XML output
├── api_enrich.py                             # CLI: phase0, prepare, merge
├── enrichment/
│   ├── base/                                 # Phase 0 output (JSON per class)
│   │   ├── Broadcaster.json
│   │   ├── Console.json
│   │   └── ...
│   ├── phase1/
│   │   ├── Broadcaster/
│   │   │   ├── Readme.md                     # Class-level artifact (durable, human-editable)
│   │   │   ├── methods_todo.md               # Workbench: progress checklist + forced type map
│   │   │   └── methods.md                    # Completed method analyses (append-only output)
│   │   ├── Console/
│   │   │   ├── Readme.md
│   │   │   ├── methods_todo.md
│   │   │   └── methods.md
│   │   └── ...
│   ├── phase1_scanned.txt                    # Diff manifest (ClassName.methodName per line)
│   ├── resources/
│   │   ├── explorations/                     # Raw C++ source explorations (Phase 1 Step A1)
│   │   │   ├── Broadcaster.md
│   │   │   └── ...
│   │   ├── guidelines/                       # Style guides for agents
│   │   │   ├── test_metadata.md
│   │   │   ├── userdocs_style.md
│   │   │   ├── diagram_creation.md
│   │   │   ├── hisescript_example_rules.md
│   │   │   ├── code_example_quality.md
│   │   │   └── builder_reference.md
│   │   ├── survey/                           # Class survey data
│   │   │   ├── class_survey.md
│   │   │   └── class_survey_data.json
│   │   ├── base_methods/                     # Pre-distilled base class methods
│   │   │   └── ScriptComponent.md
│   │   ├── laf_style_guide.json              # LAF conventions reference
│   │   └── deprecated_methods.md             # Deprecated method registry
│   ├── phase2/                               # Project example overrides
│   │   ├── Broadcaster/
│   │   │   └── addListener.md
│   │   └── ...
│   ├── phase3/                               # Author's diary (input for Phase 4a)
│   │   ├── Broadcaster/
│   │   │   ├── Readme.md                     # Class-level diary notes
│   │   │   └── addListener.md                # Method-level diary notes
│   │   └── ...
│   ├── phase4/                               # User-facing documentation (userDocs)
│   │   ├── auto/                             # LLM-generated userDocs
│   │   │   └── Console/
│   │   │       ├── Readme.md                 # Class-level prose
│   │   │       ├── print.md                  # Method-level prose
│   │   │       └── ...
│   │   └── manual/                           # Human-edited overrides (wins over auto)
│   │       └── Console/
│   │           └── print.md                  # Hand-tuned method doc
│   ├── phase4b/                              # LLM C++ reference (structured, concise)
│   │   └── Console/
│   │       ├── print.md                      # Per-method LLM reference entry
│   │       └── ...
│   └── output/
│       ├── api_reference.json                # Final merged output
│       ├── preview/                          # Plain markdown + HTML previews
│       └── website/                          # MDC-formatted markdown for Nuxt.js
```

---

## Output JSON Schema

```json
{
  "version": "1.0",
  "generated": "<ISO date>",
  "classes": {
    "<ClassName>": {
      "description": {
        "brief": "~10-15 words (required)",
        "purpose": "2-5 sentences (required)",
        "details": "Full markdown technical reference (optional, null for simple classes)",
        "obtainedVia": "Engine.createBroadcaster(defaultValues) (required)",
        "minimalObjectToken": "bc",
        "codeExample": "// basic usage (required, must create a variable named minimalObjectToken)",
        "alternatives": "Related classes (optional, null if N/A)",
        "relatedPreprocessors": ["DEFINE_NAME"],
        "userGuidePage": null,
        "userDocs": "User-facing prose (from Phase 4a, null if not yet authored)",
        "userDocOverride": false,
        "diagrams": [
          {
            "id": "diagram-id",
            "brief": "Short Human-Readable Label",
            "type": "topology",
            "description": "Plain text description of the diagram for LLM consumption",
            "svg": "diagrams/ClassName/topology_diagram-id.svg"
          }
        ]
      },
      "category": "namespace|object|component|scriptnode",
      "constants": {
        "<name>": {
          "value": 0,
          "type": "int|double|String",
          "description": "What it represents",
          "group": "Optional grouping label",
          "source": "auto|project|manual"
        }
      },
      "dynamicConstants": {
        "<name>": {
          "value": null,
          "type": "object|int",
          "description": "Structure desc, runtime-dependent",
          "source": "auto|project|manual"
        }
      },
      "commonMistakes": [
        {
          "wrong": "Incorrect code or approach",
          "right": "Correct code or approach",
          "explanation": "Why the wrong version fails",
          "source": "auto|project|manual"
        }
      ],
      "methods": {
        "<methodName>": {
          "signature": "returnType methodName(Type1 param1, Type2 param2)",
          "returnType": "Integer|Double|Number|String|bool|Array|JSON|ScriptObject|Object|undefined",
          "description": "What this method does",
          "parameters": [
            {
              "name": "param1",
              "type": "Number",
              "forcedType": true,
              "description": "What this parameter does",
              "constraints": "Optional constraints (e.g., range, valid values)"
            }
          ],
          "callScope": "safe",
          "callScopeNote": null,
          "minimalExample": "bc.addListener(obj, \"prop\", sync, fn);",
          "crossReferences": ["OtherClass.method"],
          "pitfalls": [
            { "description": "Something non-obvious", "source": "auto|project|manual" }
          ],
          "examples": [
            {
              "title": "Example title",
              "code": "// HiseScript example",
              "context": "When/why to use this",
              "source": "auto|project|manual"
            }
          ],
          "userDocs": "User-facing method prose (from Phase 4a, null if not yet authored)",
          "userDocOverride": false,
          "diagram": {
            "brief": "Short Label",
            "type": "timing",
            "description": "Plain text description of what the diagram shows",
            "svg": "diagrams/ClassName/timing_methodName.svg"
          }
        }
      }
    }
  }
}
```

**Diagram field notes:**
- Class-level `diagrams` is an **array** (a class can have multiple diagrams, each with a unique `id`).
- Method-level `diagram` is a **single object** (one diagram per method, no `id` needed).
- A method can alternatively use `"diagramRef": "class-level-diagram-id"` instead of its own `diagram` when it participates in a class-level diagram. A method has `diagram`, `diagramRef`, or neither -- never both.
- See `style-guide/scripting-api/diagrams.md` for the full diagram authoring guide.

---

## Class Description Sub-fields

The class `description` object provides three tiers of detail, allowing the MCP server to serve different levels depending on the request:

| # | Field | Required? | Source | MCP Serving Tier | Description |
|---|-------|-----------|--------|------------------|-------------|
| 1 | `brief` | **Required** | Phase 1 | **Index/search** -- return across many classes | ~10-15 words. Search-optimized summary. Token-efficient class listings. |
| 2 | `purpose` | **Required** | Phase 1 | **Working context** -- return for the target class | 2-5 sentences. Concise technical summary -- what the class does, its role, key capabilities. |
| 3 | `details` | Optional | Phase 1 | **Deep reference** -- return for deep dives | Full structured technical reference. Architecture, internal patterns, modes, inheritance-derived capabilities. Markdown with tables and headings. `null` for simple classes where `purpose` says everything. |
| 4 | `obtainedVia` | **Required** | Phase 1 | **Working context** | How to get an instance in HiseScript (e.g., `Engine.createBroadcaster()`, global variable, `Content.addX()`). |
| 5 | `minimalObjectToken` | **Required** | Phase 1 | **Working context** | Short variable name used in method `minimalExample` one-liners (e.g., `Button1`, `bc`). Empty string for namespace classes. The class `codeExample` MUST create a variable with this name. |
| 6 | `codeExample` | **Required** | Phase 1 | **Working context** | Basic usage example showing the class in action. MUST create a variable named `minimalObjectToken` (for non-namespace classes). |
| 7 | `alternatives` | Optional | Phase 1 | **Working context** | Related classes for similar tasks. `null` if N/A. |
| 8 | `relatedPreprocessors` | Optional | Phase 1 | **Deep reference** | C++ `#define` macros that gate this class's availability (e.g., `USE_BACKEND`, `HISE_INCLUDE_LORIS`). Array of strings. |
| 9 | `userGuidePage` | Deferred | -- | -- | Link to a user guide page. Not in MVP -- placeholder for later. |
| 10 | `userDocs` | Optional | Phase 4a | **Web display** | User-facing prose for the class overview. Flat string, scripter-friendly language. `null` if not yet authored. Priority: Phase 4a manual > Phase 4a auto. |
| 11 | `userDocOverride` | Auto | Phase 4a | -- | `true` if `userDocs` came from `phase4/manual/`, `false` if from `phase4/auto/` or absent. |
| 12 | `diagram` | Optional | Phase 1 | **Deep reference** + **Web display** | Diagram specification for visual rendering. Contains `type` (timing\|topology\|sequence\|state) and `description` (plain text for LLM consumption). `null` if no diagram needed. SVGs rendered in Phase 4a. |

---

## Per-Method Fields

| Field | Type | Required? | Description |
|-------|------|-----------|-------------|
| `signature` | String | **Required** | Full signature with types: `returnType methodName(Type1 param1, Type2 param2)` |
| `returnType` | String | **Required** | Return type using the VarTypes vocabulary. `undefined` for methods that return nothing. |
| `description` | String | **Required** | What this method does. |
| `parameters` | Array | **Required** | Array of parameter objects (see below). |
| `callScope` | String\|null | **Required** | Where this method can be called. One of: `"safe"` (anywhere, including audio thread), `"warning"` (audio thread OK with caveats -- see `callScopeNote`), `"unsafe"` (runtime only, not audio thread), `"init"` (onInit only -- runtime call throws script error), `null` (unknown -- treat as unsafe). |
| `callScopeNote` | String\|null | Optional | Explanation for `"warning"` tier or other non-obvious classification. Most important for `"warning"` methods where it provides concrete guidance. `null` when the classification is self-evident. |
| `minimalExample` | String | **Required** | One-liner method call with realistic arguments (e.g., `Button1.setValue(0.5);`). Written with `{obj}` placeholder in source; merge script substitutes the class's `minimalObjectToken`. |
| `crossReferences` | Array of Strings | Optional | Related methods in the format `ClassName.methodName`. |
| `pitfalls` | Array | Optional | Non-obvious behaviors or gotchas. Each entry: `{ description, source }`. |
| `examples` | Array | Optional | Code examples. Each entry: `{ title, code, context, source }`. |
| `userDocs` | String\|null | Optional | User-facing method prose from Phase 4a. `null` if not yet authored. Priority: Phase 4a manual > Phase 4a auto. |
| `userDocOverride` | boolean | Auto | `true` if `userDocs` came from `phase4/manual/`, `false` otherwise. |
| `diagram` | Object\|null | Optional | Diagram specification: `{ type, description }`. `type` is one of `timing`, `topology`, `sequence`, `state`. `description` is plain text for LLM consumption. SVGs rendered in Phase 4a. `null` if no diagram needed. |

### Parameter Object

| Field | Type | Required? | Description |
|-------|------|-----------|-------------|
| `name` | String | **Required** | Parameter name. |
| `type` | String | **Required** | Parameter type using VarTypes vocabulary. |
| `forcedType` | boolean | **Required** | `true` if extracted from `ADD_TYPED_API_METHOD_N` (authoritative). `false` if inferred by the agent. |
| `description` | String | **Required** | What this parameter does. |
| `constraints` | String | Optional | Valid ranges, accepted values, format requirements. |

### callScope Determination

The `callScope` field classifies where a method can be called, from most restricted to most free:

**`"init"`** -- Can only be called during `onInit`. The runtime enforces this -- calling at runtime throws a script error via `interfaceCreationAllowed()` or equivalent guard. Thread safety is irrelevant. Examples: `setTableMode()`, `setTableColumns()`, `setTableCallback()`.

**`"unsafe"`** -- Can be called at runtime but NOT from the audio thread. The method allocates on the heap, acquires locks, mutates ValueTree properties with change notifications, blocks, or does I/O. Call from timer callbacks, UI handlers, or other non-audio contexts. Examples: `sendData()`, component property setters via `set()`, `setLocalLookAndFeel()`.

**`"warning"`** -- Can be called from the audio thread, but with caveats described in `callScopeNote`. Sub-categories:
- **Performance-sensitive:** Lock-free but iterates over user-sized data. Fine for small collections; may need caching in tight loops. Example: `Array.indexOf()`.
- **Backend-only allocation:** Allocates in HISE IDE builds (`USE_BACKEND=1`) but compiled out or becomes a no-op in exported plugins. Fine for debugging on the audio thread since the allocation won't exist in production. Example: `Console.print()`.
- **Context-dependent:** Safe in some modes but not others. Example: `ScriptedViewport.setValue()` is safe in list mode but allocates in table MultiColumnMode.

**`"safe"`** -- No allocations, no locks, no unbounded work, no context restrictions. Call anywhere: `onInit`, runtime callbacks, audio thread, MIDI callbacks. Examples: `Math.max()`, simple cached getters, `GlobalCable.getValue()`.

**`null` (unknown)** -- Cannot be determined from the source code. Consumers should treat as `"unsafe"`.

#### Classification Rules

1. **Init-only enforcement:** If the C++ implementation checks `interfaceCreationAllowed()` or equivalent and throws a script error at runtime, classify as `"init"` regardless of what the method does internally.

2. **ValueTree property mutation:** Methods that call ValueTree set/change with notifications are `"unsafe"` -- even without explicit heap allocation, the notification chain involves string lookups, var comparisons, and potential undo-manager operations.

3. **Dispatch pattern:** When a method's own code path is lock-free and allocation-free but it dispatches/broadcasts to external targets, listeners, or callbacks, classify based on the method's own behavior. Target/listener behavior is the target's responsibility.

4. **USE_BACKEND exception:** Methods that allocate only in backend builds (`USE_BACKEND=1`) but are compiled out in exported plugins are `"warning"`, not `"unsafe"`. The allocation won't exist in production code.

5. **Subclass override:** If a subclass overrides a base method with different callScope characteristics, the subclass Phase 1 specifies the overridden value. The merge picks up the subclass value (last-writer-wins).

### Example Synthesis Heuristics

**NEEDS example:**
- Callback/function parameter
- JSON/object parameter with a schema the caller must know
- Non-obvious return structure (e.g., returns an object with specific properties)
- Multi-step workflow (method must be called in a specific sequence)
- Ambiguous method name that doesn't clearly convey usage

**SKIP example:**
- Simple getter returning a primitive value
- Simple setter with an obvious parameter
- Boolean query (e.g., `isBypassed()`)
- Self-explanatory signature where the description suffices

---

## Type System

### VarTypes Reference

Parameter and return types use HISE's `VarTypeChecker::VarTypes` names directly. This is a bitflag-based type system where composite types are bitwise ORs of their constituents.

| Type Name | Meaning | Bitflag | Composition |
|-----------|---------|---------|-------------|
| `Integer` | int, int64, bool | 1 | -- |
| `Double` | double | 2 | -- |
| `Number` | int or double | 3 | Integer \| Double |
| `String` | string | 4 | -- |
| `Colour` | hex string or numeric colour | 7 | String \| Number |
| `Array` | array | 8 | -- |
| `IndexOrArray` | single index or array of indices | 9 | Array \| Integer |
| `Buffer` | audio buffer | 16 | -- |
| `AudioData` | array or audio buffer | 24 | Array \| Buffer |
| `ObjectWithLength` | string, array, or buffer | 28 | String \| Array \| Buffer |
| `JSON` | plain JSON/dynamic object | 32 | -- |
| `ScriptObject` | any scripting API object | 64 | -- |
| `Object` | JSON or scripting API object | 96 | JSON \| ScriptObject |
| `Function` | callable function/lambda | 128 | -- |
| `ComplexType` | any non-numeric type | 252 | String \| Array \| Buffer \| JSON \| ScriptObject \| Function |
| `NotUndefined` | anything except undefined | 255 | ComplexType \| Number |

### Type Sources

**Authoritative (forcedType: true):** Methods registered with `ADD_TYPED_API_METHOD_N` macros in the C++ constructor/registration code. The macro arguments specify exact `VarTypes` per parameter. Extracted during Phase 1 class-level analysis from the same constructor code that contains `addConstant()` calls.

**Inferred (forcedType: false):** Methods registered with plain `ADD_API_METHOD_N` macros. The Phase 1 agent infers parameter types from the C++ implementation (how the `var` parameter is used -- `isString()`, `isArray()`, `getDynamicObject()`, etc.). Uses the same VarTypes vocabulary.

**Return types:** Always inferred from the implementation. Not covered by the typed macro system.

### Source C++ Reference

The type enforcement system lives in:
- **Macro definitions:** `hi_scripting/scripting/api/ScriptMacroDefinitions.h`
- **Type enum:** `hi_scripting/scripting/engine/JavascriptApiClass.h` (`VarTypeChecker::VarTypes`)
- **Runtime checking:** `hi_scripting/scripting/engine/JavascriptApiClass.cpp` (`VarTypeChecker::getType()`, `checkType()`)

Type enforcement is active in the HISE IDE (`USE_BACKEND`) and stripped from exported plugins. The `forcedType` flag in the API reference indicates which parameters have authoritative type information.

---

## Merge Rules

When multiple phases provide data for the same field, these rules determine the final value:

### Last-Writer-Wins Fields (Phase 1 > Phase 0)

These fields are set by Phase 1 (C++ analysis) and not overridden by later phases:

- `description.brief`
- `description.purpose`
- `description.details`
- `description.obtainedVia`
- `description.codeExample`
- `description.alternatives`
- `description.relatedPreprocessors`
- `description.minimalObjectToken`
- `description.diagram` (entire object replaced)
- `methods.*.description`
- `methods.*.minimalExample`
- `methods.*.diagram` (entire object replaced)
- `constants.*` (per constant, entire entry replaced)
- `dynamicConstants.*` (per constant, entire entry replaced)

### Examples (Phase 3 > Phase 2 > Phase 1)

- `methods.*.examples` (entire array replaced by the last phase that provides them)

### Merged Union Fields (all sources combined, tagged)

- `commonMistakes` -- entries from all phases, each tagged with `source`
- `methods.*.pitfalls` -- entries from all phases, each tagged with `source`; **`[BUG]`-prefixed pitfalls are excluded** (see below)
- `methods.*.crossReferences` -- all references combined, deduplicated

### Bug-Pattern Pitfall Filtering

Pitfalls that describe bugs or design issues (not intended-but-surprising behavior) are prefixed with `[BUG]` in the markdown source (e.g., `- [BUG] restoreFromBase64String does not recalculate numValues.`). These pitfalls are **excluded from `api_reference.json`** during merge. They remain in the Phase 1 `methods.md` source files as a development record and have corresponding entries in `enrichment/issues.md` for tracking the fix.

Once the bug is fixed in HISE, delete both the `[BUG]` pitfall in `methods.md` and the `issues.md` entry. The pitfall will naturally disappear from the JSON since it was already excluded.

### Source Tagging

All enrichment data is tagged with its origin:
- `"auto"` -- Phase 1 (AI-synthesized from C++ source analysis)
- `"project"` -- Phase 2 (extracted from project analysis datasets)
- `"manual"` -- Phase 3 (hand-written code examples from author's diary)

---

## Feedback Loop Prevention

**CRITICAL:** The API reference and MCP style guides are complementary, non-overlapping resources. They must not feed into each other.

- **Phase 1 class analysis (sub-agent):** Derive everything from C++ source code ONLY. Do NOT reference MCP resources, existing documentation websites, or the API reference itself.
- **Phase 1 method analysis (main agent):** May reference the MCP `hisescript-style` resource for HiseScript **syntax correctness only** (e.g., correct `inline function` syntax, proper `var` vs `const var` usage). Do NOT use it to derive API behavior or semantics.
- **MCP style guides** (`hisescript-style`, `graphics-api-style`, `scriptpanel-style`): These provide coding patterns and best practices. They reference the API but do not define it.
- **api_reference.json**: Defines what the API does. It is the authoritative source for method signatures, parameter types, and behavioral documentation.

---

## Phase 0: Doxygen XML → Base JSON

### Steps

1. Run `batchCreate.bat > NUL 2>&1` -- Runs Doxygen, copies/renames XML into `xml/selection/`, runs ApiExtractor + BinaryBuilder
2. Run `python api_enrich.py phase0` -- Parses the XML into base JSON

### Output

`enrichment/base/ClassName.json` -- one file per class

### What Phase 0 Extracts (Mechanically)

- Method names and signatures
- Return types (C++ types, to be mapped in Phase 1)
- Parameter names and C++ types
- Doxygen `@brief` descriptions (the one-line `/** */` comments from the header)
- Class category (derived from the `batchCreate.bat` rename mappings)

### What Phase 0 Does NOT Extract

- Constants -- requires reading constructor code (Phase 1)
- Forced parameter types -- requires reading `ADD_TYPED_API_METHOD_N` macros (Phase 1)
- `callScope` -- requires analyzing method implementations (Phase 1)
- Examples -- requires synthesis (Phase 1)
- `details` -- requires deep C++ source analysis (Phase 1)

### Category Mapping

Derived from `batchCreate.bat`. Each class is categorized as one of:

| Category | Description |
|----------|-------------|
| `namespace` | Global API namespaces (e.g., `Engine`, `Synth`, `Console`, `Math`) |
| `object` | Object types returned by API calls (e.g., `AudioFile`, `Broadcaster`, `MidiList`) |
| `component` | UI components created via `Content.addX()` (e.g., `ScriptButton`, `ScriptSlider`) |
| `scriptnode` | ScriptNode DSP classes |

The exact class-to-category mapping table is maintained in `scripting-api-enrichment/phase0.md`.

---

## Phase 1: C++ Source Analysis

Phase 1 is the core enrichment step. It reads C++ source code and produces structured documentation. It runs in four steps: A1 (raw exploration), A2 (distillation), B (per-method analysis), and C (post-process).

Detailed agent instructions: `scripting-api-enrichment/phase1.md`

### Steps A1 + A2 -- Sub-agent: Exploration + Distillation

Phase 1 exploration runs in two stages. See `phase1.md` for full details.

#### Step A1 -- Raw Context Gathering

**Spawned by:** The main agent, one sub-agent per class.

**Reads:** C++ source broadly -- class header, constructor, key implementations, base classes, helper classes, enum definitions, threading constraints. See the full "What to Explore" list in `phase1.md` Step A1.

**Produces:** `enrichment/resources/explorations/ClassName.md` -- raw, unfiltered context dump. This is the primary input for Step B's method analysis. Free-form markdown, prioritizes completeness over readability.

#### Step A2 -- Distillation

**Input:** The exploration file from Step A1.

**Produces two files** that feed the merge script (these are NOT context for Step B):

##### `enrichment/phase1/ClassName/Readme.md`

The durable class-level artifact. Human-editable. Contains:

```markdown
# ClassName -- Class Analysis

## Brief
~10-15 words. Search-optimized summary.

## Purpose
2-5 sentences. Concise technical summary.

## Details
Full structured technical reference. Architecture, internal patterns, modes,
inheritance-derived capabilities. Markdown with tables and headings.
(Omit this section entirely for simple classes.)

## obtainedVia
`Engine.createBroadcaster(defaultValues)` -- or however the instance is obtained.

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| ... | ... | ... | ... | ... |

## Dynamic Constants
| Name | Type | Description |
|------|------|-------------|
| ... | ... | ... |

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| ... | ... | ... |

## codeExample
```javascript
// Basic usage example
```

## Alternatives
Related classes for similar tasks, or "None."

## Related Preprocessors
`USE_BACKEND`, `HISE_INCLUDE_LORIS`, etc. -- or "None."
```

##### `enrichment/phase1/ClassName/methods_todo.md`

The workbench file. Contains the progress checklist and forced parameter type map:

```markdown
# ClassName -- Method Workbench

## Progress
- [ ] methodA
- [ ] methodB
- [ ] methodC
- [ ] methodD
...

## Forced Parameter Types
| Method | Param 1 | Param 2 | Param 3 | Param 4 | Param 5 |
|--------|---------|---------|---------|---------|---------|
| methodA | Number | -- | -- | -- | -- |
| methodC | String | Function | -- | -- | -- |
...

(Methods not listed here use plain ADD_API_METHOD_N -- types must be inferred.)
```

### Sub-agent Distillation Checklist (Step A2)

Step A2 distills the raw exploration into `Readme.md` fields. The sub-agent reads `resources/explorations/ClassName.md` and extracts:

1. **`brief`** -- What does this class do? (~10-15 words)
2. **`purpose`** -- Concise technical summary (2-5 sentences)
3. **`details`** -- Full architecture analysis (only for classes that warrant it):
   - Internal type hierarchy (e.g., base classes, inner classes, listener/target patterns)
   - Messaging or processing modes
   - State management patterns
   - Metadata systems
   - Upstream data providers -- for classes that consume external state (host transport, event sources, configuration), what provides that data? Trace the provider → dependency → API class chain, especially across build targets (backend vs frontend vs standalone). See Phase 1 Step A1 item 11 in `phase1.md`.
4. **`obtainedVia`** -- How do you get an instance? Search for factory methods, constructor registration, global variables.
5. **`codeExample`** -- Basic usage example synthesized from the class's API surface.
6. **`alternatives`** -- Related classes for similar tasks? Check sibling classes, related inheritance trees.
7. **`relatedPreprocessors`** -- Is the class gated by `#if` guards? (e.g., `USE_BACKEND`, `HISE_INCLUDE_LORIS`)
8. **`constants`** -- Extract all `addConstant(name, value)` calls from the constructor. Record name, value, and infer type from the value. For constants that serve as mode selectors (passed to configuration methods like `setSyncMode`, `setDisplayMode`, etc.), also document the behavioral impact of each value based on tracing the consuming logic through the class's dependency chain (see Phase 1 Step A1 item 12 in `phase1.md`).
9. **`dynamicConstants`** -- Document runtime-dependent constants (structure only, value=null).
10. **Trace class inheritance** -- Identify interface compatibility patterns. For example, if the class derives from `WeakCallbackHolder::CallableObject`, it can be passed wherever a callback function is expected. Document these derived capabilities in `details`.
11. **Forced parameter types** -- Extract all `ADD_TYPED_API_METHOD_N` macro invocations from the constructor. Record the method name and the `VarTypes` per parameter. Write these into `methods_todo.md`.
12. **Method checklist** -- List all API methods (from the `// API Methods` section of the header) in `methods_todo.md` as unchecked items.

**Source isolation rule:** Step A1 must derive ALL information from C++ source code only. Step A2 distills from the exploration file. Neither step may reference external documentation, MCP resources, or existing API reference files.

### Step B -- Main Agent: Sequential Method Analysis

After the sub-agent returns, the main agent processes methods one at a time.

**Loads at the start of Step B:**
- `enrichment/resources/explorations/ClassName.md` -- primary context (CRITICAL)
- `enrichment/phase1/ClassName/methods_todo.md` -- checklist + type map

Do NOT load `Readme.md` for Step B -- it is a downstream artifact for the merge script, not a context source.

**For each unchecked method:**

1. Check the forced type map in `methods_todo.md`. If the method has forced types, use them as authoritative -- no inference needed for those parameters.
2. Read the method implementation in the `.cpp` file.
3. Produce the method entry:
   - `signature` -- full signature with VarTypes
   - `returnType` -- inferred from the implementation
   - `description` -- what the method does
   - `parameters` -- name, type (forced or inferred), forcedType flag, description, constraints
   - `callScope` -- string tier or null, from implementation analysis
   - `callScopeNote` -- explanation string for non-obvious classifications (especially `"warning"`)
   - `pitfalls` -- non-obvious behaviors (if any)
   - `examples` -- synthesized code examples (apply the heuristics above)
   - `crossReferences` -- related methods (if obvious at this point; more added in post-process)
4. Append the method entry to `enrichment/phase1/ClassName/methods.md`
5. Mark the method `[x]` in `methods_todo.md`
6. Write both files to disk immediately

**The method analysis context is expendable.** After writing to disk, the agent does not need to retain any per-method details in its context window. Only the class-level context (the exploration file) and the workbench (`methods_todo.md`) are essential.

**HiseScript syntax reference:** The agent may consult the MCP `hisescript-style` resource to ensure synthesized code examples use correct HiseScript syntax. This is the ONLY external reference permitted during method analysis, and it must be used for syntax correctness only -- not for API behavior.

### Method Output Format (methods.md)

Each method entry in `methods.md` uses this markdown format:

```markdown
## methodName

**Signature:** `returnType methodName(Type1 param1, Type2 param2)`
**Return Type:** `Integer`
**Call Scope:** safe | warning | unsafe | init | unknown
**Call Scope Note:** (optional -- explanation for warning tier or non-obvious classification)

**Description:**
What this method does.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| param1 | Number | yes | What it does | 0-127 |
| param2 | Function | no | Callback | Must have 2 args |

**Pitfalls:**
- Something non-obvious about this method.

**Cross References:**
- `ClassName.relatedMethod`

**Example:**
```javascript
// Example title
// Example code here
```
```

### Compaction Recovery

The main agent's context window may be compacted automatically during long sessions. The pipeline is designed to survive this:

**After compaction, the agent MUST:**
1. Re-read `enrichment/resources/explorations/ClassName.md` -- the primary class context
2. Re-read `enrichment/phase1/ClassName/methods_todo.md` -- to find the first unchecked method
3. Do NOT re-read `enrichment/phase1/ClassName/methods.md` -- completed methods are on disk, no need to burn context

**The compaction summary should convey:** "Processing methods for ClassName. Class context is in the exploration file. Progress is in `methods_todo.md`. Reload both and continue from the first unchecked method."

### Resumability

The `methods_todo.md` file makes any session fully resumable from disk. A fresh session can:

1. Read `resources/explorations/ClassName.md` -- get the full class context
2. Read `methods_todo.md` -- see the checklist and type map
3. Find the first `- [ ]` entry -- resume from there

No conversation history, task IDs, or special recovery instructions needed. The state is entirely on disk.

### Step C -- Post-Process

After all methods for a class are complete, run the post-process step:

1. **Deduplication:** Review class `details` for content that overlaps with individual method docs. Replace verbose method descriptions in `details` with cross-references to the method entries (e.g., "See `addListener()` for the full listener registration API").

2. **Cross-reference injection:** Add `crossReferences` between related methods:
   - Deprecated methods → their replacements (e.g., `sendMessage` → `sendSyncMessage`, `sendAsyncMessage`)
   - Symmetric pairs (e.g., `setBypassed` ↔ `isBypassed`)
   - Related attach/add pairs (e.g., `attachToComponentValue` pairs with `addComponentValueListener`)

3. **Markdown → JSON transformation:** Parse `Readme.md` and `methods.md` into the output JSON schema. Produce `enrichment/phase1/ClassName.json`.

4. **Update diff manifest:** Add `ClassName.methodName` entries to `enrichment/phase1_scanned.txt` for all processed methods.

---

## Phase 2: Project Example Extraction

Phase 2 has two stages: an external extraction pipeline produces real-world examples from actual HISE projects, then the pipeline agent enriches those examples with test metadata.

### Pipeline Gate

After Phase 1 completes, check whether `enrichment/phase2/ClassName/` exists for every class in the current batch. If any directory is missing, **stop and wait** - the user will launch the extraction agent separately. Do not proceed with any class until all directories exist. Directory existence (even empty) is the completion signal.

### Test Metadata Enrichment

Once Phase 2 files exist, add slugs, test metadata blocks, setup scripts, and test-only markers to the extracted examples, then validate with `snippet_validator.py`. Do not rewrite examples beyond what's needed for testability, and do not add new examples.

### Source

- `enrichment/phase2/ClassName/Readme.md` -- class-level project context (no test metadata needed)
- `enrichment/phase2/ClassName/methodName.md` -- method examples + pitfalls

### Merge Rules

- `examples`: last-writer-wins (Phase 2 replaces Phase 1 examples entirely)
- `pitfalls`: merged union, each entry tagged `"source": "project"`
- `commonMistakes`: merged union, each entry tagged `"source": "project"`
- `crossReferences`: merged union, deduplicated
- `projectContext`: additive (Phase 2-exclusive field)

Detailed guide: `scripting-api-enrichment/phase2.md`

---

## Phase 3: Author's Diary (Input for Phase 4a)

Phase 3 is the author's diary - free-form notes capturing high-level ideas, integration patterns, domain insights, and real-world conventions. Content is written from a purpose-driven perspective ("what is this for, how do you use it"), making it especially valuable as source material for user-facing docs.

Detailed format and parsing rules: `scripting-api-enrichment/phase3.md`

### Source

- `enrichment/phase3/ClassName/Readme.md` -- class-level diary notes
- `enrichment/phase3/ClassName/methodName.md` -- method-level diary notes

Files are free-form prose (conversational, bullet points, incomplete thoughts) and optional code examples. No structured format required. The current 201 files were imported from the legacy HISE docs repository (docs.hise.dev).

### What the Merge Script Extracts

The merge script extracts two things mechanically from Phase 3 files:

- **Code examples** -- fenced code blocks, tagged `"source": "manual"`. Replace Phase 1/2 examples for the same method.
- **Cross-references** -- from markdown links `[method](/scripting/scripting-api/class#method)`. Merged with Phase 1/2 cross-references, deduplicated.

**Prose is NOT extracted.** Phase 4a agents read the raw files (injected into their prompt), extract unique insights, and rewrite in tight technical style. Phase 3 does not set `userDocs` or override any structured fields (brief, purpose, description, parameters, etc.).

---

## Phase 4a: User-Facing Documentation Authoring

Phase 4a transforms the raw C++ analysis (Phases 1-3) into scripter-friendly documentation. It runs after merge, one class at a time, because the authoring agent benefits from seeing the complete merged API surface before writing user-facing prose. Phase 4a also renders SVG diagrams for methods/classes that have a `diagram` specification.

Detailed authoring guidelines: `scripting-api-enrichment/phase4.md`

### Source

Two-tier directory structure with manual overrides winning:

```
enrichment/phase4/auto/ClassName/     # LLM-generated userDocs + auto SVGs
enrichment/phase4/manual/ClassName/   # Human-edited overrides (wins over auto)
```

### File Format

- **Class-level:** `Readme.md` -- starts with `# ClassName` heading, followed by prose paragraphs.
- **Method-level:** `methodName.md` -- bare prose, no heading. File stem must match the method name (case-insensitive matching is applied during merge).
- **Diagrams:** `methodName.svg` or `Readme.svg` -- SVG diagram rendered from the `diagram` field. Manual SVGs in `phase4/manual/` override auto-generated ones.

### JSON Fields

| Field | Level | Type | Description |
|-------|-------|------|-------------|
| `userDocs` | Class + Method | String\|null | Flat prose string for end-user display. `null` if not yet authored. |
| `userDocOverride` | Class + Method | boolean | `true` if sourced from `phase4/manual/`, `false` if from `phase4/auto/` or absent. |

### Merge Rules

- For `userDocs`: Phase 4a manual > Phase 4a auto
- `phase4/manual/` wins over `phase4/auto/` (per file, case-insensitive filename matching)
- Phase 4a does NOT override any Phase 1-3 fields -- it adds `userDocs` / `userDocOverride` and renders SVG diagrams

### Preview Output

`python api_enrich.py preview ClassName` generates:

- `ClassName_review.html` -- always generated; shows raw C++ analysis (purpose, details, description) for factual review
- `ClassName.html` -- generated only when userDocs exist; shows the user-facing documentation with auto/manual source badges

### Progress Tracking

`python api_enrich.py prepare` reports Phase 4a status by comparing files in `phase4/auto/` and `phase4/manual/` against the class's method list. No separate tracking file is needed.

---

## Phase 4b: LLM C++ Reference Authoring

Phase 4b produces structured, concise reference entries for LLMs writing HISEScript or C++ `HardcodedScriptProcessor` code. Unlike Phase 4a (which targets human scripters), Phase 4b entries are optimized for machine consumption: dense, structured, with source code call chains and explicit anti-patterns.

**Key principle:** Phase 4b requires **no new exploration**. It is a formatting pass over existing Phase 1 data -- the exploration `.md` files provide source code extracts, and `methods.md` provides structured fields. The agent reads and reformats; it does not explore.

Detailed authoring guidelines: `scripting-api-enrichment/phase4b.md`

### Source Material

| Source | What it provides |
|--------|-----------------|
| `enrichment/phase1/ClassName/methods.md` | Signature, description, callScope, parameters, pitfalls, cross-references |
| `enrichment/resources/explorations/ClassName.md` | Raw C++ source code extracts, call chains, file paths, line numbers |
| `enrichment/phase1/ClassName/Readme.md` | Class-level context (obtainedVia, constants, common mistakes) |

### Output

```
enrichment/phase4b/ClassName/methodName.md    # One file per method
```

### Entry Template

Each Phase 4b entry follows this structure:

```
ClassName::methodName(params) -> returnType

Thread safety: SAFE|WARNING|UNSAFE + explanation
[1-2 line description]
Required setup: [minimal code to call this method]
Dispatch/mechanics: [what happens internally -- 1-3 lines]
Pair with: [companion methods and why]
Anti-patterns: [what NOT to do and why]
Source:
  file.cpp:line  functionName() -> innerCall() -> deeperCall()
  [key code snippet if illuminating]
```

### Field Descriptions

| Field | Required | Description |
|-------|----------|-------------|
| Header line | Yes | `ClassName::methodName(params) -> returnType` -- C++ style, matches the scripting API signature |
| Thread safety | Yes | Maps from `callScope`: safe/warning/unsafe/init. Include the `callScopeNote` if present. |
| Description | Yes | 1-2 lines. What the method does. Terse. |
| Required setup | If needed | Minimal code showing how to obtain the object and call the method. Skip for namespace methods or trivial calls. |
| Dispatch/mechanics | If non-trivial | What happens inside -- the call chain from the scripting API wrapper through to the engine. Skip for simple getters/setters. |
| Pair with | If applicable | Companion methods. E.g., `addNoteOn` pairs with `noteOffByEventId`. |
| Anti-patterns | If applicable | What NOT to do. From pitfalls, common mistakes, and the arpeggiator exercise insights. |
| Source | Yes | File path and line number, call chain summary. From the exploration `.md`. |

### What Phase 4b Does NOT Do

- Does NOT explore C++ source code (already done in Phase 1)
- Does NOT produce prose for human readers (that's Phase 4a)
- Does NOT override any fields in `api_reference.json` -- Phase 4b entries are standalone files, not merged into the JSON
- Does NOT render diagrams (that's Phase 4a)
- Does NOT produce C++ syntax examples -- a `cppExample` field will be added post-MVP once the DLL proxy classes (issue #7) are finalized. Tracked in [hise_api_generator#13](https://github.com/christoph-hart/hise_api_generator/issues/13). Current entries use HISEScript syntax only.

### Merge Rules

Phase 4b files are **not merged into `api_reference.json`**. They are standalone reference files consumed directly by LLM agents (via MCP server or file read). This keeps Phase 4b decoupled from the main merge pipeline and allows rapid iteration on the reference format without touching the JSON schema.

### Progress Tracking

Progress is tracked by file existence: if `enrichment/phase4b/ClassName/methodName.md` exists, that method has a Phase 4b entry. The `prepare` CLI can optionally report Phase 4b coverage by comparing against the class's method list.

---

## Diff Mechanism

`enrichment/phase1_scanned.txt` tracks which methods have been processed by Phase 1.

**Format:** One `ClassName.methodName` entry per line.

**Usage:**
- `python api_enrich.py prepare` reads the base JSON and the scanned file, prints the worklist of unprocessed methods grouped by class.
- Delete a line to force rescan of that specific method.
- Delete all lines for a class to force full rescan of that class.
- The file is updated automatically during the Phase 1 post-process step.

---

## CLI Tool: api_enrich.py

### Subcommands

#### `phase0`

```
batchCreate.bat > NUL 2>&1
python api_enrich.py phase0
```

Regenerates Doxygen XML via `batchCreate.bat`, then parses all XML files in `xml/selection/` and produces base JSON files in `enrichment/base/`. Fully mechanical, no AI involvement. The batch script output is redirected to NUL to keep the agent context clean.

#### `prepare`

```
python api_enrich.py prepare
```

Reads `enrichment/base/*.json` and `enrichment/phase1_scanned.txt`. Prints the worklist of classes and methods that still need Phase 1 processing. Also reports Phase 4a (userDocs) status for all enriched classes.

#### `merge`

```
python api_enrich.py merge
```

Reads all phases (base -> phase1 -> phase2 -> phase3 -> phase4/phase4b) and produces `enrichment/output/api_reference.json`. Applies merge rules as specified above.

#### `preview`

```
python api_enrich.py preview [ClassName]
```

Generates HTML preview pages from `api_reference.json`. If a class name is given, generates only that class; otherwise generates pages for all enriched classes.

For each class, produces:
- `ClassName_review.html` -- raw C++ analysis (always generated for enriched classes)
- `ClassName.html` -- user-facing userDocs view (generated only if any Phase 4a userDocs exist for the class)
- `ClassName.md` -- plain markdown (generated only if any Phase 4a userDocs exist)

Output: `enrichment/output/preview/`

After generating all preview files, the command copies `.md` files to `enrichment/output/website/` and runs `postprocess_md.py` on them to convert plain markdown patterns into MDC (Markdown Components) format for the Nuxt.js documentation site. Transformations include converting method headings, parameter tables, warning blockquotes, see-also links, and common mistakes into their corresponding `::component` syntax.

### snippet_validator.py

```
python snippet_validator.py --validate --source auto --class ClassName --launch
```

Validates code examples by executing them in HISE. Filters by `--source` (auto/project/manual/all) and `--class`. The `--launch` flag auto-starts HISE Debug if not already running. Results are written to `enrichment/output/test_results.json` and incorporated into the merged JSON via `api_enrich.py merge`.

