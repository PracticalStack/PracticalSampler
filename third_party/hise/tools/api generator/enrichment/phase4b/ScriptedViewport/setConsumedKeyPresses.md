ScriptedViewport::setConsumedKeyPresses(NotUndefined listOfKeys) -> undefined

Thread safety: UNSAFE
Defines which key presses this component consumes. Accepts "all", "all_nonexclusive", a JSON key description object, or an array of these. Must be called BEFORE setKeyPressCallback.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  vp.setConsumedKeyPresses("all");
Dispatch/mechanics: Parses key descriptions into internal KeyPress list. "all" catches all keys exclusively; "all_nonexclusive" catches all but lets parent still receive them. JSON objects specify keyCode, shift, cmd, alt, character.
Pair with: setKeyPressCallback (registers the handler; must be called after this method)
Anti-patterns: Must be called BEFORE setKeyPressCallback -- reports a script error if called after.
Source:
  ScriptingApiContent.cpp  ScriptComponent::setConsumedKeyPresses() -> parseKeyDescription()
