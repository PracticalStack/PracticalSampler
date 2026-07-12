# Phase 1: C++ Source Analysis -- Agent Instructions

Phase 1 is the core enrichment step. It reads C++ source code and produces structured documentation for each class. It runs in three stages with distinct execution models.

**Source isolation rule:** ALL information must be derived from C++ source code ONLY. Do NOT reference external documentation or the API reference itself. The only permitted external references are the resource files in `enrichment/resources/` (see Resource Files below).

**ASCII-only rule:** All Phase 1 output files (`Readme.md`, `methods_todo.md`, `methods.md`) and all resource files must use ASCII characters only. Use `--` instead of em-dashes, straight quotes instead of curly quotes, etc. The Write tool on Windows may produce CP1252 instead of UTF-8 for non-ASCII characters, which breaks the merge script's UTF-8 parser.

**Write tool only rule:** NEVER use bash commands (`echo`, `cat`, heredocs), Python scripts, or any other shell-based method to write or append to output files. Always use the Write tool for new files and the Edit tool for modifications. Bash-based writes corrupt markdown (backtick escaping, quote mangling) and produce encoding issues. If a Write call fails, retry it -- do not fall back to bash. Note: Steps A1 and A2 use the `explore-hise-cpp` agent which has Write/Edit tools but no bash -- this rule is enforced structurally. Step B uses `general` agents which have bash access and must follow this rule explicitly.

---

## Resource Files

The `enrichment/resources/` folder contains reference material that agents
consult during Phase 1. These files are shared across all class enrichments.

| File | Purpose | Used by |
|------|---------|---------|
| `resources/survey/class_survey.md` | Class survey: prerequisite table, enrichment groups, factory relationships | Step A1 (prerequisites) |
| `resources/survey/class_survey_data.json` | Machine-readable survey data: features, cross-references, seeAlso pairs | Step A1 (prerequisites) |
| `style-guide/scripting-api/hisescript-rules.md` | HISEScript syntax, callback, and LAF rules for code examples | Step B |
| `resources/laf_style_guide.json` | LAF callback property definitions | Step B |
| `resources/deprecated_methods.md` | Deprecated method registry with C++ macro status | Step B |
| `resources/base_methods/*.md` | Pre-distilled method entries for base classes (e.g. ScriptComponent) | Step B |
| `resources/explorations/*_base.md` | Raw exploration output for base classes | Step A1, Step B |
| `resources/explorations/ClassName.md` | Raw exploration output for individual classes | Step B |
| `style-guide/scripting-api/*.md` | Style guidelines for userDocs, code examples, diagrams | Step B, Phase 4a |

---

## Step A1 -- Sub-agent (explore): Raw Context Gathering

**Execution model:** One sub-agent per class. Reads C++ source broadly and deeply, dumps everything a method-analysis agent might need.

**Output:** `enrichment/resources/explorations/ClassName.md`

This is a RAW output -- no distillation, no summarization. The goal is to gather every piece of context that Step B method agents might need. Be too thorough rather than not thorough enough.

### What to Explore

1. **Class declaration** -- full header analysis: inheritance chain, inner types, nested classes, listener/target patterns
2. **Internal class hierarchies** -- e.g. for ScriptedViewport: the `ListBoxModel` subclass that contains most of the table logic
3. **Helper classes and utility functions** that methods call into -- e.g. `ApiHelpers::convertStyleSheetProperty` for `setStyleSheetProperty`
4. **Enum definitions** -- string constant tables, switch/case patterns, valid value sets
5. **JSON schema construction patterns** -- `DynamicObject` construction, `setProperty()` calls, `NamedValueSet` usage
6. **Constructor** -- `addConstant()` calls, `ADD_TYPED_API_METHOD_N` registrations
7. **Factory methods / obtainedVia** -- how do you get an instance?
8. **Threading / lifecycle constraints** -- onInit-only restrictions, thread safety
9. **Preprocessor guards** -- `#if USE_BACKEND`, `HISE_INCLUDE_LORIS`, etc.
10. **Trace class inheritance** -- interface compatibility patterns (e.g. `WeakCallbackHolder::CallableObject`)
11. **Upstream data providers** -- For classes that depend on external state (clock sources, event producers, host data, configuration systems), identify what *provides* data to the class's key dependencies. Trace the provider → dependency → API class chain, especially across build targets (backend vs frontend vs standalone). Example: TransportHandler depends on MasterClock; MasterClock receives external clock data from `ExternalClockSimulator` (standalone) or the DAW `AudioPlayHead` (plugin). This upstream context is critical infrastructure that affects the meaning of the entire API surface.
12. **Enum/constant behavioral tracing** -- For constants that act as mode selectors or configuration values (sync modes, display modes, processing modes), do not just capture the enum definition and comments. Trace each value through the functions that consume it and document the behavioral consequences. Identify all decision points where the value is checked (`if`/`switch` statements across the class and its dependency chain) and summarize the runtime behavior per value. This is infrastructure context, not individual method analysis.

### What to Skip

Do NOT deep-dive individual method implementations line-by-line. That is Step B's job. Focus on the infrastructure and context that surrounds the methods. **Exception:** When tracing mode-selector constants (item 12) or upstream data providers (item 11), you may need to read implementations of helper/dependency methods in other classes to understand the behavioral impact or data flow. This is subsystem-level infrastructure context, not individual method analysis -- it belongs in Step A1.

### Prerequisite Context: Class Survey

Before exploring, consult `resources/survey/class_survey.md` for the target class. The **Enrichment Prerequisites** table (15 entries) lists conceptual dependencies where understanding class A materially improves the documentation of class B. These are NOT mere factory relationships -- they are cases where A introduces concepts, conventions, or patterns that B needs to reference.

Also consult `resources/survey/class_survey_data.json` for the target class entry to get:
- **creates**: What this class produces (helps you understand its role)
- **seeAlso**: Related classes with distinction text (helps write cross-references)
- **createdBy**: Which factory class creates this one

**Prerequisite loading rule:** Check the prerequisite table in `resources/survey/class_survey.md`. If the target class appears in the "Then Enrich" column, the "Enrich First" class provides essential architectural context. Load that prerequisite's `phase1/ClassName/Readme.md` into context before starting exploration.

The prerequisite describes the broader system this class operates within. Your exploration should reference the prerequisite's architectural model rather than re-discovering it, focus on what THIS class adds or specializes, and note where this class's behavior depends on the prerequisite's mechanisms. For example:
- When enriching `MacroHandler`, load `UserPresetHandler`'s Readme -- it describes the preset save/load lifecycle that macros participate in
- When enriching `GlobalCable`, load `GlobalRoutingManager`'s Readme -- it describes OSC conventions and cable topology that GlobalCable inherits
- When enriching `Expansion`, load `ExpansionHandler`'s Readme -- it describes pack lifecycle and credential model that Expansion instances operate within
- When enriching `Node`, load `DspNetwork`'s Readme -- it describes the graph model and node creation patterns that Node instances live in

### Existing Resources to Consult

Also check `enrichment/resources/explorations/` for existing base class explorations:

- **Component classes** (category `"component"`): Consult `resources/explorations/ScriptComponent_base.md` for the ScriptComponent base class context. Do not re-explore ScriptComponent -- focus on child-class-specific infrastructure.
- **Complex data classes** (ScriptTable, ScriptSliderPack, ScriptAudioWaveform): Consult the ComplexDataScriptComponent exploration if it exists.
- **Any other base class**: Check for a matching `*_base.md` file in `resources/explorations/`.

List all resource files consulted (including prerequisite Readmes) at the top of the output.

### Output Format

Free-form markdown. Use headings to organize by topic. Include code snippets, tables, enum listings -- whatever captures the information faithfully. This file is consumed by agents, not humans, so prioritize completeness over readability.

### Large Exploration Files

Complex classes (Broadcaster, TransportHandler, Engine, Synth, etc.) routinely produce explorations of 800-1200 lines. This is expected and should not be truncated -- later phases depend on thorough context.

To avoid Write tool failures on large files, use incremental writes:

1. Write the first ~500 lines using the Write tool (class declaration, constructor, enums, infrastructure)
2. Use the Edit tool to append subsequent sections of ~300-500 lines each (helper classes, upstream providers, constant tracing)
3. Never attempt to write more than ~800 lines in a single Write tool call

If a Write call fails or produces a truncated file, do NOT fall back to bash. Retry using a smaller chunk via Edit-append.

---

## Step A2 -- Distillation: Readme.md + methods_todo.md

**Execution model:** Same agent or a separate sub-agent. Reads the raw exploration from A1 and produces two lightweight files.

**Input:** `enrichment/resources/explorations/ClassName.md`

**Output:** Two files in `enrichment/phase1/ClassName/`:

1. **`Readme.md`** -- Class-level content that feeds into the JSON. Only carries the fields needed by the merge script.
2. **`methods_todo.md`** -- Workbench: progress checklist + forced parameter type map.

The Readme does NOT carry context for Step B. That role belongs to the raw exploration file.

### Readme.md Template

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

## minimalObjectToken
bc

(Short variable name for method minimal examples. Empty for namespace classes.
The `codeExample` below MUST create a variable with this name.)

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| ... | ... | ... | ... | ... |

## Dynamic Constants
| Name | Type | Description |
|------|------|-------------|
| ... | ... | ... |

## Common Mistakes

Entries must describe code that is genuinely broken or produces unintended behavior -- not code that works correctly under default assumptions. If a method has a sensible default and calling it without explicit configuration produces valid, predictable results, that is not a mistake. Awareness items about defaults belong in the Purpose/Details section or in the relevant method description.

| Wrong | Right | Explanation |
|-------|-------|-------------|
| ... | ... | ... |

## codeExample
```javascript
// Basic usage example. MUST create a variable named after minimalObjectToken.
// e.g. const var bc = Engine.createBroadcaster({...});
```

## Alternatives
Related classes for similar tasks, or "None."

## Related Preprocessors
`USE_BACKEND`, `HISE_INCLUDE_LORIS`, etc. -- or "None."

## Diagrams

### diagram-id
- **Brief:** Short Human-Readable Label
- **Type:** topology
- **Description:** Plain text description of what the diagram shows. This serves
  as the LLM-facing representation. Phase 4a renders it as an SVG for humans.

(Omit this section entirely if no class-level diagrams are needed.
 Add multiple h3 sub-sections for multiple diagrams.)

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: One sentence explaining why no methods in this class warrant parse-time
diagnostics (e.g., no timeline dependencies, no silent-failure preconditions, etc.)
```

### methods_todo.md Template

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

---

## Step B -- Main Agent: Sequential Method Analysis

After Step A completes, the main agent processes methods one at a time.

### Context Loading

At the start of Step B, load these files (in priority order):

1. `enrichment/resources/explorations/ClassName.md` -- primary context (CRITICAL)
2. `enrichment/phase1/ClassName/methods_todo.md` -- checklist + type map
3. `enrichment/resources/deprecated_methods.md` -- deprecated method list
4. Relevant base class resources:
   - For `"component"` classes: `resources/explorations/ScriptComponent_base.md` and `resources/base_methods/ScriptComponent.md`
   - For complex data classes: the relevant base class exploration and base_methods file
5. `style-guide/scripting-api/hisescript-rules.md` -- for code examples
6. `enrichment/resources/laf_style_guide.json` -- for LAF code examples (component classes)

Do NOT load `Readme.md` for Step B -- it is a downstream artifact for the merge script, not a context source.

### Inherited Base Methods (Component Classes)

All UI component classes inherit ~34 methods from the ScriptComponent base class. Pre-distilled entries for these methods exist in `resources/base_methods/ScriptComponent.md`.

**For each inherited method, the agent makes a judgment call:**

1. **Adopt** -- The base method is relevant and behaves the same on this child class. Copy the entry from `base_methods/ScriptComponent.md` into this class's `methods.md`. Optionally add class-specific notes if needed.

2. **Override** -- The child class overrides this method (check the Virtual Method Override Summary in the base_methods file). Re-analyze the child's override implementation from C++ source and document behavioral differences. Write a fresh entry.

3. **Disable** -- The method exists on this class but is not useful. Write a minimal disabled entry (see Disabled Methods below). Common disable reasons:
   - The base implementation is a trivial pass-through only meaningful on a specific overriding class (e.g. `setValueNormalized` is only useful on ScriptSlider)
   - The method depends on a property that is deactivated for this component type
   - The method is listed in `resources/deprecated_methods.md`

This avoids redundant C++ analysis of the same base methods for every component class while still producing accurate, class-specific documentation.

### Disabled Methods

A method gets disabled when it exists on the class but should not appear in user-facing documentation. There are four structured reasons:

| Reason | Meaning | Example |
|--------|---------|---------|
| `no-op` | Method does nothing useful on this class | `setValue` on ScriptMultipageDialog |
| `redundant` | Base impl is trivial pass-through; only meaningful on overriding class | `setValueNormalized` on non-ScriptSlider |
| `deprecated` | Superseded by a better API (listed in `resources/deprecated_methods.md`) | `setColour` |
| `property-deactivated` | Depends on a property deactivated for this component | `addToMacroControl` when macroControl is deactivated |

Disabled entries are minimal -- just the flag and reason, no signature or parameters. See the disabled entry template in the methods.md Output Format section below.

**Deprecated methods:** Check `resources/deprecated_methods.md` for any method matching `ClassName.methodName(N)`. If a match is found, use reason `deprecated` and copy the rationale from the deprecated file.

If you discover a deprecated method during C++ analysis that is NOT yet in `deprecated_methods.md`, add it there with `Status: pending`, the argument count `(N)`, and the `Reason:` suggestion string (the exact text for the C++ `ADD_API_METHOD_N_DEPRECATED` macro). Also check the constructor for existing `ADD_API_METHOD_N_DEPRECATED` macro usage -- if present, mark as `Status: applied`.

### Per-Method Workflow

For each unchecked method in `methods_todo.md`:

1. **Check if it is an inherited base method.** If so, follow the Adopt/Override/Disable decision above.
2. **Check the deprecated list.** If the method appears in `resources/deprecated_methods.md`, write a disabled entry.
3. **Check the forced type map.** If the method has forced types, use them as authoritative -- no inference needed for those parameters.
4. **Read the method implementation** in the `.cpp` file. Use the raw exploration for surrounding context.
5. **Produce the method entry** (see methods.md Output Format below).
6. **Log bugs.** If any pitfall describes a bug or design issue (not just intended-but-surprising behavior), append an entry to `enrichment/issues.md`. See Bug & Issue Tracking below.
7. **Check for diagnostic opportunities.** Does this method silently fail without prior setup? Does it require another method to have been called first? Could a parse-time check catch a common user mistake? If yes, append an entry to `enrichment/diagnostic-ideas.md`. See Diagnostic Ideas Side-Channel below.
8. **Append** the method entry to `enrichment/phase1/ClassName/methods.md`
9. **Mark the method `[x]`** in `methods_todo.md`
10. **Write both files to disk immediately**

### HISEScript Syntax & Example Rules

All code examples MUST follow `style-guide/scripting-api/hisescript-rules.md` (loaded in Step B context). That file is authoritative -- do not write examples without consulting it.

### VarTypes Vocabulary

Parameter and return types must use these names:

| Type Name | Meaning | Bitflag |
|-----------|---------|---------|
| `Integer` | int, int64, bool | 1 |
| `Double` | double | 2 |
| `Number` | int or double | 3 |
| `String` | string | 4 |
| `Colour` | hex string or numeric colour | 7 |
| `Array` | array | 8 |
| `IndexOrArray` | single index or array of indices | 9 |
| `Buffer` | audio buffer | 16 |
| `AudioData` | array or audio buffer | 24 |
| `ObjectWithLength` | string, array, or buffer | 28 |
| `JSON` | plain JSON/dynamic object | 32 |
| `ScriptObject` | any scripting API object | 64 |
| `Object` | JSON or scripting API object | 96 |
| `Function` | callable function/lambda | 128 |
| `ComplexType` | any non-numeric type | 252 |
| `NotUndefined` | anything except undefined | 255 |

**For return types:** Use `undefined` for methods that return nothing (void in C++).

### Type Inference Rules

For methods registered with plain `ADD_API_METHOD_N` (no forced types):

1. Look at how the `var` parameter is used in the implementation
2. `isString()` / `toString()` -- `String`
3. `isArray()` / `getArray()` -- `Array`
4. `getDynamicObject()` -- `JSON`
5. `isInt()` / `isInt64()` / `(int)` cast -- `Integer`
6. `isDouble()` / `(double)` cast -- `Double`
7. Numeric operations without specific type -- `Number`
8. `isBool()` -- `Integer` (booleans are Integer in VarTypes)
9. Function parameters (callbacks) -- `Function`
10. If multiple types are accepted (e.g., `isString()` or `isArray()`), use the appropriate composite type

### callScope Determination

Classify where a method can be called. The tiers form a spectrum from most restricted to most free:

**`"init"`** -- Can only be called during `onInit`. Look for `interfaceCreationAllowed()` checks or equivalent guards that throw a script error at runtime. If the C++ enforces init-only, classify as `"init"` regardless of what the method does internally.

**`"unsafe"`** -- Can be called at runtime but NOT from the audio thread. Examine the C++ for:
- Heap allocations (`new`, `String` construction, `Array::add`)
- Lock acquisitions (mutex, critical section, `ScopedLock`)
- ValueTree property set/change with notifications (notification chain involves string lookups, var comparisons, potential undo-manager)
- I/O operations or blocking

**`"warning"`** -- Can be called from the audio thread, but with caveats. Always provide a `callScopeNote`. Three sub-categories:
- **Performance-sensitive:** Lock-free but iterates over user-sized data (e.g., `indexOf` on an array). Note the O(n) characteristic.
- **String involvement:** Method accepts or returns a `String`, which involves atomic ref-count operations on the `StringHolder`. No heap allocation occurs for copies/moves, but atomics cause cache-line contention under concurrent access. Classify as `warning` with note "String involvement, atomic ref-count operations."
- **Context-dependent:** Safe in some modes/configurations but not others (e.g., a method that is lock-free in one mode but allocates in another).

**`"safe"`** -- No allocations, no locks, no unbounded work. Call anywhere: `onInit`, runtime, audio thread, MIDI callbacks.

**`null`** -- Cannot be determined. Set only when the source is genuinely ambiguous after thorough analysis. Consumers treat as `"unsafe"`.

#### Additional Rules

- **Dispatch pattern:** When a method dispatches to external targets/callbacks but its own path is lock-free, classify based on the method's own code. Target behavior is the target's responsibility.
- **Subclass override:** If a subclass overrides a base method with different characteristics, write the subclass-specific callScope. The merge picks it up (last-writer-wins).
- **Error path exclusion:** `reportScriptError()` on the error path does not affect classification. Classify based on the non-error execution path only. Error reporting is an exceptional condition that the scripter should not be hitting in production code.
- **Compiled-out methods:** Methods that are no-ops in exported plugins (e.g., all `Console` methods) are classified as `safe`. The classification reflects the shipped plugin behaviour, not the HISE IDE debug build. If the method does real work only in backend builds, that is irrelevant to the audio-thread safety of the exported product.

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

### Test Metadata for Examples

When synthesizing an example, also produce `testMetadata` following `style-guide/scripting-api/test-metadata.md`. That file defines the full schema, verification types, and setup patterns.

### Diagram Heuristic

Writing a diagram description is cheap (2-3 sentences + type tag). Phase 4a decides which get rendered. **When in doubt, write the description. Do not self-censor.** See `style-guide/scripting-api/diagrams.md` for type definitions, JSON schema, and conventions.

### Minimal Example Conventions

Every non-disabled method gets a `**Minimal Example:**` field -- a single one-liner showing the method call with realistic arguments. These are required and distinct from the optional full `**Example:**` code blocks.

**Rules:**

1. **Always use `{obj}` as the object token.** The merge script substitutes the class's `minimalObjectToken` automatically at merge time. Do not hard-code the variable name.
2. **Namespace class methods do not use `{obj}`.** Write the call using the namespace name directly: `Console.print("hello");`, `Engine.getSampleRate();`.
3. **Show return value assignment** when the method returns something: `var id = {obj}.getId();`. Use `var` (not `const var` or `local`) for the assignment.
4. **Callback parameters:**
   - **Realtime-safe methods:** Use a preexisting reference: `{obj}.setControlCallback(onMyControl);`
   - **Non-realtime methods:** Use inline `=>` syntax where it fits one line: `Engine.showYesNoWindow("title", "msg", ok => Console.print(ok));`
   - If `=>` syntax would make the line too long, use a preexisting reference instead.
5. **No comments, no `const var` declarations, one line only.**
6. **Literal values** for simple arguments, **assumed preexisting objects** for complex ones (`laf`, `myCallback`, `myData`).
7. **Colours:** Use `Colours.xxx` constants where a simple colour suffices (e.g., `Colours.red`). Use `0xAARRGGBB` format when a specific colour is needed.
8. **Disabled methods:** Skip -- no `**Minimal Example:**` field.

### Structured Value Documentation

These optional sections capture machine-readable metadata that downstream consumers (MCP server, autocomplete, docs website) use for validation and hallucination prevention. They go in the method entry after `**Parameters:**` and before `**Pitfalls:**`.

**String enum parameters (`valueDescriptions`):** When any parameter accepts a fixed set of string values -- whether as a single string or as an array of strings -- add a `**Value Descriptions:**` section with a `| Value | Description |` table. Each row documents one valid value and a brief plain-English description of when it applies. Pull descriptions from the exploration's behavioral analysis of each value (see Step A1 item 12). If the exploration traced the constant through its consuming functions, summarize the behavioral consequences -- not just the C++ enum comment. Each value description should answer: "What changes in the class's behavior when this value is active?" If the exploration only contains verbatim enum comments without behavioral context, provide the best description possible from available context and add a comment noting the gap. The valid values should still be listed in the parameter `Constraints` column for backward compatibility.

**Callback and config object properties (`callbackProperties`):** When a method accepts a callback function whose argument is a structured object, or accepts a JSON configuration object with specific keys, add a `**Callback Properties:**` section with a `| Property | Type | Description |` table. Extract exact property names from the C++ `setProperty()`, `DynamicObject` construction, or `NamedValueSet` calls -- do not guess. This is the primary defense against property name hallucination (e.g. `columnId` vs `columnID`).

Both sections are optional -- only add them when the method actually has string enums or structured objects.

### Callback Signature

When a method accepts a `Function` parameter (a callback), add a `**Callback Signature:**` field documenting the expected parameter count, names, and types. This enables the LSP to validate callback argument counts at parse time.

**Format:**

```
**Callback Signature:** parameterName(argName1: type1, argName2: type2)
```

Where `parameterName` matches the Function parameter name from the `**Parameters:**` table. Use C++/HISEScript-idiomatic type names (`double`, `int`, `bool`, `var`, `String`, `Object`).

**How to extract from C++ source:**

1. Find the `WeakCallbackHolder` constructor for this callback. The last argument is the expected parameter count: `WeakCallbackHolder(scriptProcessor, thisObject, callbackVar, expectedNumArgs)`
2. Find the `callback.call1(value)`, `callback.call(args, N)`, or `callback.callSync(args, N)` invocation. The arguments passed reveal the parameter names and types.
3. For `call(args, N)` patterns, look at the `args[0] = ...`, `args[1] = ...` assignments just above the call to get names and types.
4. Some callbacks (e.g. `setControlCallback`) use `executeInlineFunction` instead of `WeakCallbackHolder`. In these cases, check the argument array construction at the call site.

**Example entries:**

```
**Callback Signature:** callbackFunction(value: double)
**Callback Signature:** controlFunction(component: ScriptComponent, value: var)
**Callback Signature:** sortFunction(a: var, b: var)
**Callback Signature:** keyboardFunction(event: Object)
```

This field is required for every method that has a `Function` parameter. If the callback takes a structured object as its argument (e.g., an event object with named properties), also add a `**Callback Properties:**` table documenting the object's properties.

### Pitfall Quality Rules

A pitfall documents behavior that the user cannot self-diagnose from HISE's runtime feedback alone. Apply these filters before writing a `**Pitfalls:**` entry:

- **Silent failure = pitfall.** Method accepts input, appears to succeed, but does not work. No error message, no warning. The user has no way to diagnose this without reading C++ source.
- **Misleading behavior = pitfall.** Method succeeds but produces wrong or unexpected results under specific conditions.
- **Unexpected side effect = pitfall.** Calling method X also resets or changes Y with no indication.
- **Clear error message = NOT a pitfall.** If HISE throws a script error with a descriptive message (e.g., "SineGenerator1 wasn't found"), the user can self-diagnose. Do not document expected error handling as a pitfall.
- **Implementation details = NOT a pitfall.** Internal code paths, property read ordering, or default values that behave as expected are not pitfalls. A pitfall must describe a user-facing consequence.
- **Coalesce related pitfalls.** If multiple pitfalls describe the same failure mechanism with different triggers, combine them into one entry. One pitfall per failure *mechanism*, not per trigger.
- **Bug or design issue?** If a behavior looks like a bug rather than intended behavior, also log it in `enrichment/issues.md` (see Bug & Issue Tracking section below). Write both the pitfall (documenting current behavior for users) and the issue entry (tracking the fix). **Prefix bug-pattern pitfalls with `[BUG]`** in the methods.md bullet item (e.g., `- [BUG] restoreFromBase64String does not recalculate numValues.`). The `[BUG]` prefix signals the merge script to exclude this pitfall from the user-facing `api_reference.json` -- it will be removed once the underlying bug is fixed in HISE. Non-bug pitfalls (surprising-but-intended behavior) must NOT have the `[BUG]` prefix.

### methods.md Output Format

Each method entry uses this format:

```markdown
## methodName

**Signature:** `returnType methodName(Type1 param1, Type2 param2)`
**Return Type:** `Integer`
**Call Scope:** safe | warning | unsafe | init | unknown
**Call Scope Note:** (optional -- explanation for warning tier or non-obvious classification)
**Minimal Example:** `{obj}.methodName(42, onMyCallback);`

**Description:**
What this method does.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| param1 | Number | yes | What it does | 0-127 |
| param2 | Function | no | Callback | Must have 2 args |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "Selection" | A row is selected |
| "DoubleClick" | A cell receives a double click |

**Callback Signature:** param2(component: ScriptComponent, value: var)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| Type | String | The event type name |
| rowIndex | Integer | The row index in the display order |

**Pitfalls:**
- Something non-obvious about this method.

**Cross References:**
- `$API.ClassName.relatedMethod$`

**Diagram:**
- **Brief:** Short Human-Readable Label
- **Type:** timing
- **Description:** Plain text description of what the diagram shows.

**DiagramRef:** diagram-id

(Use **Diagram:** for method-owned diagrams, **DiagramRef:** for cross-references
to a class-level diagram. Use one or neither, never both.
Omit if no diagram needed -- see Diagram Heuristic below.)

**Example:**
```javascript
// Example title
// Example code here
```
```

Disabled method entry format (no `**Minimal Example:**` line needed):

```markdown
## methodName

**Disabled:** no-op | redundant | deprecated | property-deactivated
**Disabled Reason:** Brief explanation of why this method is disabled on this class.
```

---

## Bug & Issue Tracking

During C++ analysis you will encounter behavior that is a bug, design issue, or silent failure -- not intended behavior to document. **Log every such finding in `enrichment/issues.md`** immediately when discovered (Per-Method Workflow step 6).

This is separate from pitfalls: pitfalls document current behavior for users; issues track things that should be fixed in HISE itself. Do not assume issues will be fixed -- document current behavior accurately in the method entry regardless. If an issue also affects users today, write both a pitfall and an issue entry.

**Pitfall/issue pairing rule:** Every bug-pattern pitfall in `methods.md` must have:
1. A `[BUG]` prefix on the pitfall bullet item in `methods.md`
2. A corresponding entry in `enrichment/issues.md` under the correct severity heading

The `[BUG]` prefix causes the merge script to exclude the pitfall from `api_reference.json`. Once the bug is fixed in HISE, delete both the `[BUG]` pitfall and the issues.md entry.

**Entry format:**

```
### ClassName.methodName -- short description

- **Type:** silent-fail | missing-validation | inconsistency | code-smell | ux-issue
- **Severity:** critical | high | medium | low
- **Location:** SourceFile.cpp:~line
- **Observed:** What happens now.
- **Expected:** What should happen instead.
```

Append new issues under the correct severity heading in `enrichment/issues.md`.

---

## Diagnostic Ideas Side-Channel

During C++ analysis you may notice patterns where a method silently fails, behaves unexpectedly, or produces confusing results when called without proper setup -- but the behavior is *by design*, not a bug. These are opportunities for LSP diagnostics that catch user mistakes at parse time via `addDiagnostic()` query lambdas. Log these in `enrichment/diagnostic-ideas.md`.

Look for:

- **Timeline dependencies:** Method B requires Method A to have been called first (e.g., `setBaseURL` before `callWithGET`, `setTableMode` before `setTableCallback`)
- **State preconditions:** Methods that need a specific mode/flag enabled to do useful work
- **Value validation:** String/identifier parameters that could be checked against known sets at parse time
- **Combination checks:** Mutually exclusive settings, redundant calls that overwrite each other

This is separate from both pitfalls and issues: pitfalls document behavior for users; issues track bugs; diagnostic ideas track opportunities for automated parse-time warnings where the underlying HISE behavior is correct.

When you find a diagnostic opportunity during method analysis (Per-Method Workflow step 7), append an entry to `enrichment/diagnostic-ideas.md` under the correct priority heading using the entry template in that file.

### Diagnostic Ideas Attestation

After all methods are analyzed, the `## Diagnostic Ideas` section in `Readme.md` must be filled in:

- **If ideas were found:** Set `Count:` to the number logged and list each as a bullet with method name and category (e.g., `- ScriptedViewport.setTableCallback -- timeline dependency (logged)`).
- **If no ideas were found:** Set `Count: 0` and write a `Rationale:` line -- one sentence explaining why none of the methods in this class warrant parse-time diagnostics. The rationale must be specific to this class (e.g., "All methods operate on immediate values with no inter-method state dependencies" or "All failure modes already produce descriptive runtime error messages"). Generic dismissals like "No diagnostics needed" are not acceptable.

This attestation is a gate condition in Step C (see Diagnostic Ideas Attestation Check below).

---

## Step C -- Post-Process

After all methods for a class are complete:

### 1. Deduplication

Review class `details` in the Readme for content that overlaps with individual method docs. Replace verbose method descriptions in `details` with cross-references to the method entries (e.g., "See `addListener()` for the full listener registration API").

### 2. Cross-Reference Injection

Add `crossReferences` between related methods using canonical `$DOMAIN$` link tokens (see `style-guide/canonical-links.md`):
- Deprecated methods and their replacements
- Symmetric pairs (e.g., `setBypassed` and `isBypassed`)
- Related attach/add pairs (e.g., `attachToComponentValue` pairs with `addComponentValueListener`)
- Cross-domain links to modules or docs (e.g., `$MODULES.Convolution$`, `$DOC.Architecture.ModuleTree$`)

### 3. Markdown to JSON Transformation

Parse `Readme.md` and `methods.md` into the output JSON schema. This is done by `api_enrich.py merge` -- no manual step needed. The merge command reads the markdown files and produces `enrichment/output/api_reference.json`.

### 4. Update Diff Manifest

Add `ClassName.methodName` entries to `enrichment/phase1_scanned.txt` for all processed methods.

### 5. Bug/Issue Consistency Check

Scan all pitfalls in `methods.md`:
- Every pitfall with a `[BUG]` prefix must have a corresponding entry in `enrichment/issues.md`
- Every pitfall describing a bug, silent failure, or wrong behavior that is NOT tagged `[BUG]` should be reviewed: if it's genuinely a bug, add the `[BUG]` prefix and create the issues.md entry
- Every `[BUG]` pitfall that does NOT have a matching issues.md entry: create the entry now

This check is a gate -- the class is not considered complete until bug/issue pairs are consistent.

### 6. Diagnostic Ideas Attestation Check

Verify the `## Diagnostic Ideas` section in `Readme.md`:

- The section must exist with `Reviewed: Yes`
- If `Count: 0`: a `Rationale:` line must be present and non-empty. Generic dismissals (e.g., "No diagnostics needed", "None found") are not acceptable -- the rationale must be specific to this class.
- If `Count: N` (N > 0): there must be N bullet items listing each idea with method name and category, and each must have a corresponding entry in `enrichment/diagnostic-ideas.md`

This check is a gate -- the class is not considered complete until the diagnostic attestation is present and valid.

---

## Session Prompts

### Single class (most common)

```
Follow tools/api generator/doc_builders/scripting-api-enrichment.md. Run phase0, then run phase1 for Console.
```

### Resume interrupted class

```
Follow tools/api generator/doc_builders/scripting-api-enrichment.md. Resume phase1 for Broadcaster.
```

### Post-process only

```
Follow tools/api generator/doc_builders/scripting-api-enrichment.md. Run post-process for Broadcaster.
```
