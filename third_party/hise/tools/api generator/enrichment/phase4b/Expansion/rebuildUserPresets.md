Expansion::rebuildUserPresets() -> bool

Thread safety: UNSAFE -- file I/O for ValueTree loading, potential decryption, and user preset file extraction to disk
Extracts user presets from encoded expansion data to the filesystem. Only works with
Intermediate or Encrypted expansion types. Overwrites existing presets (forceExtraction = true).
Returns true on success.

Required setup:
  const var e = Engine.createExpansionHandler().getExpansionList()[0];

Dispatch/mechanics:
  dynamic_cast<ScriptEncryptedExpansion*>(exp)
    -> loadValueTree(v) (may involve decryption)
    -> extractUserPresetsIfEmpty(v, forceExtraction=true)

Anti-patterns:
  - Do NOT call on FileBased expansions -- silently returns false with no error message.
    Check getExpansionType() first.
  - Do NOT expect existing user preset modifications to survive -- forceExtraction
    overwrites all preset files unconditionally.

Source:
  ScriptExpansion.cpp:1827  ScriptExpansionReference::rebuildUserPresets()
    -> dynamic_cast to ScriptEncryptedExpansion (fails for FileBased -> returns false)
    -> loadValueTree() + extractUserPresetsIfEmpty(v, true)
