Settings::stopPerfettoTracing(ScriptObject traceFileToUse) -> undefined

Thread safety: UNSAFE -- ends the Perfetto session and writes trace data to disk
Stops the active Perfetto tracing session and writes trace data to the specified
file. Requires PERFETTO=1. The file must have a .pftrace extension.

Anti-patterns:
  - [BUG] File extension is validated after the trace session has already ended and
    data written. Wrong extension causes a script error but the data is saved to
    the misnamed file. Perfetto tools may not recognize it without .pftrace.

Pair with:
  startPerfettoTracing -- begin a tracing session

Source:
  ScriptingApi.cpp  Settings::stopPerfettoTracing()
    -> MelatoninPerfetto::get().endSession(true) [PERFETTO guard]
    -> validates .pftrace extension after write
