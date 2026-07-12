ScriptButton::setPopupData(JSON jsonData, Array position) -> undefined

Thread safety: UNSAFE
Attaches a FloatingTile configuration to this button. Clicking the button toggles
a popup containing the configured FloatingTile. Click again to dismiss.

Dispatch/mechanics:
  Stores jsonData and parses position via ApiHelpers::getIntRectangleFromVar
  On click: HiToggleButton::mouseDown creates FloatingTile with stored JSON,
  shows as root popup at popupPosition offset relative to the button

Anti-patterns:
  - The position array must be [x, y, w, h] -- throws script error
    "position must be an array with this structure: [x, y, w, h]" if wrong
  - Buttons already inside a FloatingTilePopup will not create nested popups

Source:
  ScriptingApiContent.cpp:2711  ScriptButton::setPopupData()
    -> ApiHelpers::getIntRectangleFromVar(position)
    -> stores popupData and popupPosition for HiToggleButton::mouseDown
