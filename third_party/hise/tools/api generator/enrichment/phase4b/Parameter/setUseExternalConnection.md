Parameter::setUseExternalConnection(Integer usesExternalConnection) -> undefined

Thread safety: UNSAFE -- modifies CachedValue and ValueTree properties with undo manager.

Configures whether setValue() dispatches to the async (dynamicParameter->call()) or
sync (ValueTree with undo) path. When set to true, removes the Value property from
the parameter's ValueTree -- the value lives only in dynamicParameter and is not
persisted. When set to false, restores the Value property from dynamicParameter's
displayValue or the parameter's default. Call once during setup, then use setValue()
for all value changes.

Dispatch/mechanics:
  true:  CachedValue write -> removes Value property from ValueTree (with undo)
  false: CachedValue write -> restores Value property using dynamicParameter
         displayValue (if available) or DefaultValue property (with undo)

Pair with:
  setValue -- the unified setter that reads the externalConnection flag

Anti-patterns:
  - Do NOT switch from external to non-external (false) without understanding value
    restoration -- the restored Value property uses the current DSP display value if
    the dynamic parameter is initialized, otherwise falls back to DefaultValue. If
    neither is meaningful, the parameter may reset to an unexpected value.

Source:
  NodeBase.cpp  Parameter::setUseExternalConnection()
    -> writes CachedValue<bool> externalConnection
    -> true: data.removeProperty(PropertyIds::Value, parent->getUndoManager())
    -> false: data.setProperty(PropertyIds::Value, restoredValue, parent->getUndoManager())
