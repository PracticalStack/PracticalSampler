ScriptAudioWaveform::grabFocus() -> undefined

Thread safety: UNSAFE
Notifies z-level listeners that the component wants to grab keyboard focus.
Only notifies the first listener (exclusive operation).

Pair with:
  loseFocus -- to release keyboard focus

Source:
  ScriptingApiContent.cpp  ScriptComponent::grabFocus()
    -> triggers wantsToGrabFocus() on first ZLevelListener
