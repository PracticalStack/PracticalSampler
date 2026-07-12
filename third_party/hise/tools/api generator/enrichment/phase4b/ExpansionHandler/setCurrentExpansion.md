ExpansionHandler::setCurrentExpansion(var expansionName) -> bool

Thread safety: UNSAFE -- state change with async notifications, potential preset
save/restore via FullInstrumentExpansion.
Sets the active expansion by name (String) or Expansion reference. Triggers the
expansion callback. On first activation, saves default state for later restoration.
Pass empty string to clear the current expansion.
Dispatch/mechanics:
  String arg: ExpansionHandler::setCurrentExpansion(name)
  Expansion ref: extracts Name property, delegates to String overload
    -> first activation: FullInstrumentExpansion::setNewDefault() saves state
    -> checks HISE version compatibility
    -> sends ExpansionLoaded notification via AsyncUpdater
Pair with:
  setExpansionCallback -- register before first call to receive initial switch
  getCurrentExpansion -- read back the active expansion
  getExpansion -- look up by name before setting
Anti-patterns:
  - Do NOT set the expansion callback after the first setCurrentExpansion() call --
    the callback won't fire for the initial switch
  - Do NOT pass a non-String, non-Expansion type -- reports "can't find expansion"
    without indicating the type mismatch
Source:
  ScriptExpansion.cpp:1293  setCurrentExpansion()
    -> ExpansionHandler::setCurrentExpansion(String) (ExpansionHandler.cpp:340)
    -> setCurrentExpansion(Expansion*) (ExpansionHandler.cpp:363)
