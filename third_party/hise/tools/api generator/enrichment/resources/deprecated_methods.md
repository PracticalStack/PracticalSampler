# Deprecated Methods

Methods discovered during Phase 1 analysis that should use the
ADD_API_METHOD_N_DEPRECATED macro in their C++ constructor.

Format:

```
### ClassName.methodName(N)
Status: pending | applied
Reason: "suggestion text for the macro"

One-sentence rationale.
```

- N = argument count (maps to ADD_API_METHOD_N_DEPRECATED).
- Reason = exact string that goes in the macro's text parameter.
- Status: applied = macro already in C++. pending = waiting to be added.
- New entries are added here as they are discovered during Phase 1 class
  enrichment. Do not run separate sweeps -- discovery is a side effect of
  the per-class C++ analysis.

---

### Graphics.drawText(2)
Status: applied
Reason: "use drawAlignedText for better placement"

Superseded by drawAlignedText which supports alignment options.

### Broadcaster.sendMessage(2)
Status: pending
Reason: "use sendSyncMessage or sendAsyncMessage instead"

The second parameter (`isSync` bool) is ambiguous and hard to guess. The C++ implementation already emits a `debugError` at runtime but does not use the `ADD_API_METHOD_N_DEPRECATED` macro.

### Synth.noteOff(1)
Status: pending
Reason: "use noteOffByEventId instead"

Note-off by note number is unreliable with overlapping voices on the same pitch. The implementation calls `addNoteOff` then `reportScriptError` under `ENABLE_SCRIPTING_SAFE_CHECKS`, but does not use the `ADD_API_METHOD_N_DEPRECATED` macro. Also, the error message has a typo: "noteOfByEventId" instead of "noteOffByEventId".

### Synth.setUseUniformVoiceHandler(2)
Status: pending
Reason: "global envelopes are now automatic, remove this call"

Fully deprecated. The implementation immediately calls `reportScriptError("This function is deprecated. Just remove that call and enjoy global envelopes...")` and performs no work. Does not use the `ADD_API_METHOD_N_DEPRECATED` macro.

### Engine.getSettingsWindowObject(0)
Status: pending
Reason: "use the Settings class instead"

Hard deprecated. The implementation calls `reportScriptError("Deprecated")` and returns `var()`. Does not use the `ADD_API_METHOD_N_DEPRECATED` macro.

### Engine.getZoomLevel(0)
Status: pending
Reason: "use Settings.getZoomLevel() instead"

Soft deprecated. The implementation calls `logSettingWarning("getZoomLevel")` which emits a console message, then proceeds to return the value from `GlobalSettingManager::getGlobalScaleFactor()`. The method still works but warns users to migrate to the Settings class.

### Parameter.setValueAsync(1)
Status: applied
Reason: "Use setUseExternalConnection(), then setValue() instead."

Superseded by the unified `setValue()` + `setUseExternalConnection()` pair. Marked via `DIAGNOSTIC_MARK_DEPRECATED` macro in the constructor.

### Parameter.setValueSync(1)
Status: applied
Reason: "Use setUseExternalConnection(), then setValue() instead."

Superseded by the unified `setValue()` + `setUseExternalConnection()` pair. Marked via `DIAGNOSTIC_MARK_DEPRECATED` macro in the constructor.

### ExpansionHandler.setEncryptionKey(1)
Status: pending
Reason: "Use the project settings to setup the project's blowfish key"

Hard deprecated. The implementation calls `reportScriptError("This function is deprecated. Use the project settings to setup the project's blowfish key")` and performs no work. Does not use the `ADD_API_METHOD_N_DEPRECATED` macro.

### Content.setToolbarProperties(1)
Status: pending
Reason: "deprecated since 2017"

Hard deprecated. The implementation immediately calls `reportScriptError("2017...")` and performs no work. Does not use the `ADD_API_METHOD_N_DEPRECATED` macro (Content uses DynamicObject::setMethod registration).

### Engine.loadFont(1)
Status: pending
Reason: "use loadFontAs() instead to prevent cross platform issues"

Soft deprecated. The implementation calls `debugError(getProcessor(), "loadFont is deprecated. Use loadFontAs() instead to prevent cross platform issues")` then delegates to `loadFontAs(fileName, String())`. The method still works but the OS-dependent font name resolution makes it unreliable across platforms.

### Engine.setDiskMode(1)
Status: pending
Reason: "use Settings.setDiskMode() instead"

Soft deprecated. The implementation calls `logSettingWarning("setDiskMode")` which emits a console message, then proceeds to set the disk mode. The method still works but warns users to migrate to the Settings class.

### Engine.setZoomLevel(1)
Status: pending
Reason: "use Settings.setZoomLevel() instead"

Soft deprecated. The implementation calls `logSettingWarning("setZoomLevel")` which emits a console message, then proceeds to set the zoom level. The method still works but warns users to migrate to the Settings class.

### Engine.getZoomLevel(0)
Status: pending
Reason: "use Settings.getZoomLevel() instead"

Soft deprecated. The implementation calls `logSettingWarning("getZoomLevel")` which emits a console message, then proceeds to return the value from `GlobalSettingManager::getGlobalScaleFactor()`. The method still works but warns users to migrate to the Settings class.
