ExpansionHandler::setExpansionCallback(var expansionLoadedCallback) -> undefined

Thread safety: UNSAFE -- creates a WeakCallbackHolder on the heap, increments ref
count, registers as source.
Sets a callback that fires when an expansion is loaded, created, or cleared. Receives
an Expansion object when activated/discovered, or undefined when cleared.
Callback signature: f(var expansion)
Pair with:
  setCurrentExpansion -- triggers the callback on expansion change
  refreshExpansions -- triggers the callback when new expansions are discovered
Anti-patterns:
  - Do NOT register this callback after the first setCurrentExpansion() call --
    the initial switch notification will be missed
Source:
  ScriptExpansion.cpp  setExpansionCallback()
    -> expansionCallback WeakCallbackHolder with setThisObject + addAsSource
    -> expansionPackLoaded/expansionPackCreated both dispatch through this
