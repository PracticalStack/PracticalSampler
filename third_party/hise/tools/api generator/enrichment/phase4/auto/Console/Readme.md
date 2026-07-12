# Console

Console provides debugging and diagnostic tools for HISEScript development in the HISE IDE. The class offers three main capabilities:

1. Output methods for logging values and clearing the console.
2. Assertion methods for validating conditions and data types during development.
3. Profiling tools for benchmarking code and sampling data.

Use assertions as guard clauses to surface programming errors immediately rather than letting bad state propagate silently. The profiling tools help you measure execution time and capture data snapshots for inspection in the code editor.

> [!Tip:No-ops in exported plugins, thread-safe] All Console methods become no-ops in exported plugins, so you can leave debugging calls in production code without performance cost. Console methods are safe to call from any thread, including the audio thread.

## Common Mistakes

- **Use assertions for programming errors**
  **Wrong:** `if (!isDefined(obj)) return;` in a function where `obj` must always exist  
  **Right:** `Console.assertIsDefined(obj);`  
  *Silent early returns hide bugs. If `obj` being undefined is a programming error (not a valid state), an assertion surfaces it immediately during development while being stripped in release builds.*

- **Console methods are no-ops in exports**
  **Wrong:** Wrapping `Console.print()` calls in `if (Engine.isHISE())` guards  
  **Right:** Call `Console.print()` directly without guards  
  *Console methods are already no-ops in exported plugins. Manual guards add clutter with no benefit.*

- **Start benchmark before stopping**
  **Wrong:** `Console.stopBenchmark()` without calling `Console.startBenchmark()` first  
  **Right:** Call `Console.startBenchmark()` before `Console.stopBenchmark()`  
  *`stopBenchmark` throws a script error if the benchmark was never started.*

- **Start sampling session before sampling**
  **Wrong:** Calling `Console.sample()` without `Console.startSampling()`  
  **Right:** Call `Console.startSampling("id")` before `Console.sample()`  
  *Without an active session, `sample()` logs a warning and skips recording. The warning is only shown once.*
