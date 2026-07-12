Effect::restoreState(String base64State) -> undefined

Thread safety: UNSAFE -- suspends audio processing, kills voices, and waits for audio thread clearance before restoring. Heavy operation.
Restores the full processor state from a Base64 string previously obtained via
exportState(). Reports a script error if the string cannot be parsed.
Dispatch/mechanics:
  SuspendHelpers::ScopedTicket -> killVoicesAndExtendTimeOut()
    -> LockHelpers::freeToGo() (waits for audio thread)
    -> ProcessorHelpers::restoreFromBase64String(effect, base64State)
Pair with:
  exportState -- obtain the Base64 string to restore from
Anti-patterns:
  - Do NOT expect UI to update automatically after restoreState() -- it does
    not send attribute notifications. Re-set parameters or call
    updateValueFromProcessorConnection() on connected components to sync UI.
Source:
  ScriptingApiObjects.cpp:3373  ScriptingEffect::restoreState()
    -> ScopedTicket + killVoicesAndExtendTimeOut + freeToGo + restoreFromBase64String
