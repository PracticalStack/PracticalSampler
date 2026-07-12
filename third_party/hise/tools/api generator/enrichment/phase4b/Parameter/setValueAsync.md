DEPRECATED: Use setValue() + setUseExternalConnection(true).

Parameter::setValueAsync(Double newValue) -> undefined

Thread safety: SAFE
Sets the parameter value immediately via dynamicParameter->call(). Applies
to all voices in polyphonic networks via NoVoiceSetter. Does not update the
parameter's ValueTree and does not support undo.

Dispatch/mechanics:
  DspNetwork::NoVoiceSetter(rootNetwork) -> dynamicParameter->call(newValue)
    -> direct function pointer dispatch to the DSP node (lock-free)

Pair with:
  setValueSync -- when undo support or ValueTree persistence is needed
  getValue -- read back the current value

Anti-patterns:
  - Do NOT assume the call always reaches DSP -- if dynamicParameter is nullptr
    (node not yet initialized), the call is silently ignored.

Source:
  NodeBase.cpp:1063  Parameter::setValueAsync()
    -> NoVoiceSetter nvs(*parent->getRootNetwork())
    -> dynamicParameter->call(newValue)
