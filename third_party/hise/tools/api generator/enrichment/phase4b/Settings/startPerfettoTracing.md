Settings::startPerfettoTracing() -> undefined

Thread safety: UNSAFE -- begins a Perfetto profiling session
Starts a Perfetto performance tracing session. Requires HISE to be compiled with
PERFETTO=1. Throws a script error if Perfetto is not enabled.

Pair with:
  stopPerfettoTracing -- end session and write trace file

Source:
  ScriptingApi.cpp  Settings::startPerfettoTracing()
    -> MelatoninPerfetto::get().beginSession() [PERFETTO guard]
