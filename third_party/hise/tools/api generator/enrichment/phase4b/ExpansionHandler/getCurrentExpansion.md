ExpansionHandler::getCurrentExpansion() -> ScriptObject

Thread safety: UNSAFE -- allocates a ScriptExpansionReference wrapper on the heap.
Returns the currently active expansion, or undefined if no expansion is active.
Pair with:
  setCurrentExpansion -- sets the active expansion
Source:
  ScriptExpansion.cpp  getCurrentExpansion()
    -> wraps currentExpansion WeakReference as ScriptExpansionReference
