Content::setValuePopupData(var jsonData) -> undefined

Thread safety: SAFE -- simple var assignment to a member variable.
Configures the global appearance of value popups that appear when interacting with knobs
and sliders. Applies globally to all components in this Content instance.

JSON properties:
  fontName (String, default "Default"), fontSize (float, default 14.0),
  borderSize (float, default 2.0), borderRadius (float, default 2.0),
  margin (float, default 3.0), bgColour (hex, default 0xFFFFFFFF),
  itemColour (hex, default 0xaa222222), itemColour2 (hex, default 0xaa222222),
  textColour (hex, default 0xFFFFFFFF)

Source:
  ScriptingApiContent.cpp:8692  Content::setValuePopupData()
    -> stores JSON, consumed by ScriptCreatedComponentWrapper::ValuePopup::Properties
