ScriptAudioWaveform::setKeyPressCallback(Function keyboardFunction) -> undefined

Thread safety: UNSAFE
Registers a callback for key press events when this component has focus.
The callback receives an event object with key or focus change properties.

Anti-patterns:
  - MUST call setConsumedKeyPresses() BEFORE this method -- reports script error otherwise

Pair with:
  setConsumedKeyPresses -- must be called first to define which keys to consume

Source:
  ScriptingApiContent.cpp  ScriptComponent::setKeyPressCallback()
    -> checks consumedKeyPresses is set -> stores keyboardCallback
