Settings::crashAndBurn() -> undefined

Thread safety: UNSAFE -- deliberately crashes the process via null pointer dereference
Deliberately crashes the process for testing crash reporting and stack traces.
In backend builds, throws a script error first if CompileWithDebugSymbols is not
enabled, giving a chance to enable debug symbols for a meaningful stack trace.

Dispatch/mechanics:
  Backend: checks CompileWithDebugSymbols setting, reportScriptError if disabled
  All builds: volatile int* x = nullptr; *x = 90; abort();

Anti-patterns:
  - Do NOT call in production/frontend builds without understanding that it crashes
    unconditionally with no prerequisite check

Pair with:
  setEnableDebugMode -- enable debug logging before crash testing

Source:
  ScriptingApi.cpp  Settings::crashAndBurn()
    -> GET_HISE_SETTING(CompileWithDebugSymbols) [USE_BACKEND only]
    -> null pointer dereference + abort()
