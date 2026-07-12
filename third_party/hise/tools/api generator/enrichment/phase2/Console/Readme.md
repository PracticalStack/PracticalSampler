# Console -- Project Context

## Project Context

### Real-World Use Cases
- **Development-time validation**: Assertion methods (`assertTrue`, `assertEqual`, `assertIsDefined`, `assertIsObjectOrArray`, `assertNoString`) are used extensively as guard clauses in utility functions, module tree builders, and data-binding layers to catch configuration errors early. A plugin with a Builder API pipeline validates each created module with `assertIsDefined` immediately after creation, and uses `assertEqual` to verify computed data array sizes match expected dimensions before processing.
- **Configuration self-checks**: `assertEqual` and `assertTrue` validate preprocessor definitions and file existence at init time, catching misconfigured project settings before they cause obscure failures at runtime. A plugin's activation system uses `assertEqual` to verify that `USE_COPY_PROTECTION` and `USE_SCRIPT_COPY_PROTECTION` are both set to `1`, and `assertTrue` to confirm that the RSA key file exists on disk.
- **Progress and state logging**: `Console.print` is used pervasively to trace state machine transitions (export pipelines, preset loading sequences, file operations) during development. Most of these calls are left in the codebase because they become no-ops in exported plugins.
- **Benchmark profiling**: `startBenchmark`/`stopBenchmark` is used to measure initialization-time operations like waveform processing.

### Complexity Tiers
1. **Basic debugging** (universal): `print` for logging values, `clear` to reset the console before a multi-step operation. Every plugin uses these.
2. **Defensive assertions** (common in structured codebases): `assertTrue`, `assertIsDefined`, `assertIsObjectOrArray` as guard clauses in utility functions and module tree construction. Plugins with Builder API pipelines or custom data-binding systems use these heavily.
3. **Unit-test-style validation** (advanced): `assertEqual` for verifying computed values against expected results, `assertWithMessage` for descriptive failure messages, `assertNoString`/`assertLegalNumber` for type-safety checks. Used in initialization routines that validate structural invariants.

### Practical Defaults
- Leave `Console.print` calls in production code freely. They become no-ops in exported plugins, so there is no performance cost and they serve as built-in documentation of control flow.
- Use `Console.assertTrue(false)` as an "unreachable code" marker in functions that must always return from a loop or switch. This is clearer than a bare `return undefined` and immediately surfaces logic errors during development.
- Prefer `Console.assertIsDefined(obj)` over manual `isDefined` checks when the absence of a value indicates a programming error rather than a normal state. The assertion halts execution with a clear error, while a manual check silently propagates `undefined`.
- Use `Console.clear()` before starting a multi-step operation (batch export, preset conversion) to isolate its output from prior noise.

### Integration Patterns
- `Console.assertEqual()` with `Engine.getExtraDefinitionsInBackend()` - Validates preprocessor definitions at init time to catch misconfigured project settings before export.
- `Console.assertIsDefined()` after `Builder.create()` / `Builder.get()` - Guards every module creation step in a Builder API pipeline, ensuring that newly created modules are valid before setting their attributes.
- `Console.assertIsObjectOrArray()` after `Synth.getEffect()` / `Synth.getChildSynth()` - Validates that module lookups return real objects before attempting to call methods on them.
- `Console.assertTrue(false)` at function end - Marks unreachable code paths in lookup functions that iterate data arrays and must find a match.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `if (!isDefined(obj)) return;` in a function where `obj` must always exist | `Console.assertIsDefined(obj);` | Silent early returns hide bugs. If `obj` being undefined is a programming error (not a valid state), an assertion surfaces it immediately during development while being stripped in release builds. |
| Wrapping `Console.print()` calls in `if (Engine.isHISE())` guards | Call `Console.print()` directly without guards | Console methods are already no-ops in exported plugins. Manual guards add clutter with no benefit. |
