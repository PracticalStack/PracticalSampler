Console::sample(String label, NotUndefined dataToSample) -> undefined

Thread safety: SAFE
Records a labelled data snapshot into the active sampling session. Data is cloned at capture time, so subsequent mutations do not affect the recorded value. If no session is active (no `Console.startSampling()` call), a one-time warning is printed and the call is skipped. Shows inline debug value in code editor. Requires `HISE_INCLUDE_PROFILING_TOOLKIT`; otherwise a no-op.

Anti-patterns:
- The "no session started" warning is only shown once per Console instance. Subsequent calls without a session are silently skipped -- easy to miss.

Pair with: Console.startSampling -- must be called first to create the session.
