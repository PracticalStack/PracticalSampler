Console::stop(Integer condition) -> undefined

Thread safety: SAFE
Cooperative breakpoint that halts script execution when `condition` is true (non-zero). On scripting/sample-loading/audio threads, suspends via `JavascriptThreadPool::ScopedSleeper`, rebuilds debug info, and waits for IDE resume. On the message (UI) thread, reports a script error instead of suspending. Timeout is extended by the suspension duration.

Anti-patterns:
- Do not use on the message thread -- reports "Breakpoint in UI Thread" error instead of suspending, because blocking the UI would freeze the application.
- While paused, the audio thread outputs silence by design.

Pair with: Console.breakInDebugger -- triggers a native C++ debugger breakpoint instead of HISEScript suspension.
