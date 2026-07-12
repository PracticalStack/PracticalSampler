Console::stopBenchmark() -> undefined

Thread safety: SAFE
Stops the benchmark timer started by `Console.startBenchmark()` and prints elapsed time in ms (3 decimal places). Reports a script error if `startBenchmark()` was not called first. Only produces output in backend builds.

Anti-patterns:
- Do not call `stopBenchmark()` without a preceding `startBenchmark()` -- throws a script error.
- No output in exported plugins; backend-only.

Pair with: Console.startBenchmark -- must be called first to start the timer.
