Threads::getCurrentThreadName() -> String

Thread safety: WARNING -- String return value involves atomic ref-count operations.
Returns a human-readable name for the current thread. Equivalent to
Threads.toString(Threads.getCurrentThread()).

Dispatch/mechanics:
  Inline in header: return toString(getCurrentThread())

Pair with:
  getCurrentThread -- returns the integer constant instead of the string name

Source:
  ScriptingApi.h:1860  getCurrentThreadName() const { return toString(getCurrentThread()); }
