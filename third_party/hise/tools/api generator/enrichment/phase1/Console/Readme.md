# Console -- Class Analysis

## Brief
Debug utility providing console output, assertions, benchmarking, and data sampling for HISEScript.

## Purpose
`Console` is a global scripting API class (`hise::ScriptingApi::Console`) that provides debugging and diagnostic tools for HISEScript development. It wraps the HISE backend console output system, offering methods for printing messages, timing code with start/stop benchmarks, asserting conditions during development, triggering native debugger breakpoints, and recording data sampling sessions via the profiling toolkit. All Console methods are permitted on the audio thread (via `allowIllegalCallsOnAudioThread` returning `true`), though most output-producing methods are effectively no-ops in frontend (exported plugin) builds due to `#if USE_BACKEND` guards in their implementations.

## Details
The class is lightweight and stateless beyond a benchmark timer (`startTime`), a debug location tracker (`id`/`lineNumber`), and a `ProfileCollection` for the profiling toolkit integration. The `setDebugLocation` method is called automatically by the engine before each Console method invocation (when `ENABLE_SCRIPTING_BREAKPOINTS` is defined), injecting the calling script's file identifier and line number so that `print`, `stop`, `blink`, `startSampling`, and `sample` can reference the source location.

A notable parser-level optimization exists: when `ENABLE_SCRIPTING_SAFE_CHECKS` is disabled (i.e., in release/CI builds), the parser detects `Console.*` calls and replaces the entire expression with a no-op, completely eliminating Console overhead from compiled scripts.

The `stop()` method implements a cooperative breakpoint system: on the scripting/sample-loading/audio thread it uses `JavascriptThreadPool::ScopedSleeper` to suspend execution, while on the UI thread it reports an error. The `blink()` method sends a visual flash to the code editor at the calling line.

## obtainedVia
Global namespace object. Registered via `scriptEngine->registerApiClass(new ScriptingApi::Console(this))` in each `JavascriptProcessor` subclass constructor. Accessed in HISEScript as `Console.methodName()`.

## Constants
(None -- `ApiClass(0)` is called with zero constants, and no `addConstant` calls exist in the constructor.)

| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|

## Dynamic Constants
(None)

| Name | Type | Description |
|------|------|-------------|

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Console.stopBenchmark()` without calling `Console.startBenchmark()` first | Call `Console.startBenchmark()` before `Console.stopBenchmark()` | `stopBenchmark` checks if `startTime` is 0.0 and reports a script error if the benchmark was never started. |
| Relying on `Console.print()` output in exported plugins | Use `Console.print()` only during development in the HISE IDE | `print()` is gated by `#if USE_BACKEND`; in frontend builds it only calls `DBG()` (debug macro), which is stripped in release. |
| Using `Console.stop()` on the message thread | Use `Console.stop()` from scripting/audio/sample-loading threads | On the UI thread, `stop()` reports a script error ("Breakpoint in UI Thread") rather than suspending execution. |
| Calling `Console.sample()` without `Console.startSampling()` | Call `Console.startSampling("id")` before `Console.sample()` | Without an active session, `sample()` logs a warning and skips. The warning is only shown once per Console instance (`warnIfNoSession` flag). |

## codeExample
```javascript
// Basic printing
Console.print("Hello from HISEScript");

// Benchmarking a block of code
Console.startBenchmark();
for (var i = 0; i < 1000; i++)
    Math.sin(i);
Console.stopBenchmark();

// Assertions for debugging
Console.assertTrue(myValue > 0);
Console.assertEqual(expected, actual);
Console.assertIsDefined(someObject);
Console.assertLegalNumber(gainValue);
Console.assertIsObjectOrArray(data);
Console.assertNoString(numericParam);
Console.assertWithMessage(x > 0, "x must be positive");

// Breakpoint - halts execution (scripting thread only)
Console.stop(shouldBreak);

// Blink the current line in the code editor
Console.blink();

// Clear the console output
Console.clear();

// Data sampling session (requires profiling toolkit)
Console.startSampling("mySession");
Console.sample("iteration", currentData);
```

## Alternatives
- `Engine.logSettingWarning(message)` -- for logging warnings through the Engine API rather than the Console.
- The `trace()` keyword / inline debug values -- the engine's built-in inplace debug display system (separate from Console).

## Related Preprocessors
- `USE_BACKEND` -- Guards the implementation of `print()`, `stopBenchmark()`, and `blink()`. In frontend builds, `print()` falls back to `DBG()`, and the other methods become no-ops.
- `HISE_INCLUDE_PROFILING_TOOLKIT` -- Guards the data sampling/profiling code path inside `print()`, `startSampling()`, and `sample()`.
- `ENABLE_SCRIPTING_BREAKPOINTS` -- Controls whether `setDebugLocation()` is called before each Console method invocation (engine-level, not in Console itself).
- `ENABLE_SCRIPTING_SAFE_CHECKS` -- When disabled, the parser strips all `Console.*` calls entirely as a performance optimization.
