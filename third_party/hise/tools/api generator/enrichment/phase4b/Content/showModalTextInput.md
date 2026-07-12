Content::showModalTextInput(var properties, Function callback) -> undefined

Thread safety: UNSAFE -- creates a TextInputData object (heap allocation) and broadcasts asynchronously to the message thread.
Shows a modal text input overlay on the interface. The callback fires when the user
presses Return (ok=true), Escape (ok=false), or the editor loses focus (ok=false).
The text editor is automatically removed after the callback fires.
Callback signature: f(bool ok, String text)

JSON properties:
  x (int), y (int), width (int), height (int), text (String, default ""),
  alignment (String, default "centred"), bgColour (hex, default 0x88000000),
  itemColour (hex, default 0), textColour (hex, default 0xAAFFFFFF),
  fontName (String), fontStyle (String, default "plain"),
  fontSize (float, default 13.0), parentComponent (String)

Dispatch/mechanics:
  Creates TextInputData -> textInputBroadcaster.sendMessage(async)
  Listener creates TextEditor with configured properties
  TextEditorListener handles Return/Escape/focus-lost -> fires callback

Source:
  ScriptingApiContent.cpp:8966  Content::showModalTextInput()
    -> TextInputData construction
    -> textInputBroadcaster (LambdaBroadcaster<TextInputDataBase::Ptr>)
    -> TextEditorListener dispatches callback with (bool ok, String text)
