Content::setKeyPressCallback(var keyPress, var callback) -> undefined

Thread safety: UNSAFE -- parses key press description (String operations, KeyPress construction) and modifies the registeredKeyPresses array.
Registers a keyboard shortcut callback at the interface level. The keyPress can be a
string description (e.g., "ctrl + s", "shift + F5") or a JSON object with keyCode,
character, shift, cmd/ctrl, alt properties. Pass a non-function as callback to unregister.
Callback signature: f(Object event)

Callback event properties:
  isFocusChange (bool), character (String), specialKey (bool), isWhitespace (bool),
  isLetter (bool), isDigit (bool), keyCode (int), description (String),
  shift (bool), cmd (bool), alt (bool)

Dispatch/mechanics:
  keyPress string -> KeyPress::createFromDescription()
  keyPress JSON -> manual KeyPress construction from properties
  Valid function -> adds/replaces in registeredKeyPresses array
  Non-function -> removes matching key press registration

Pair with:
  ScriptComponent.setKeyPressCallback -- per-component key handling

Source:
  ScriptingApiContent.cpp:9135  Content::setKeyPressCallback()
    -> ApiHelpers::getKeyPress() for parsing
    -> registeredKeyPresses array management
    -> Content::handleKeyPress() dispatches via createKeyboardCallbackObject()
