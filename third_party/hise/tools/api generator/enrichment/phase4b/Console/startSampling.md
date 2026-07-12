Console::startSampling(String sessionId) -> undefined

Thread safety: SAFE
Starts a data sampling session with the given identifier. Subsequent `Console.sample()` calls record data snapshots into this session. Managed by the profiling toolkit's `DebugSession` system. Shows inline debug value in code editor. Requires `HISE_INCLUDE_PROFILING_TOOLKIT`; otherwise a no-op.

Pair with: Console.sample -- records labelled data snapshots into the active session.
