ScriptImage::setKeyPressCallback(Function keyboardFunction) -> undefined

Thread safety: UNSAFE
Registers a callback for key press events when this component has focus.
MUST call setConsumedKeyPresses() BEFORE this method.
Callback receives an event object with either key press properties (isFocusChange=false,
character, specialKey, keyCode, description, shift, cmd, alt) or focus change
properties (isFocusChange=true, hasFocus).
Required setup:
  const var img = Content.addImage("MyImage", 0, 0);
  img.setConsumedKeyPresses("all");
Pair with:
  setConsumedKeyPresses -- must be called first to define consumed keys
Anti-patterns:
  - Do NOT call without setConsumedKeyPresses() first -- triggers script error
Source:
  ScriptingApiContent.cpp  ScriptComponent::setKeyPressCallback()
    -> checks consumedKeyPresses is set -> stores keyboard function
