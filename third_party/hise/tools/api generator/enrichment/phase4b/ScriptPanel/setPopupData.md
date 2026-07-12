ScriptPanel::setPopupData(JSON jsonData, Array position) -> undefined

Thread safety: UNSAFE -- configures FloatingTile data and popup bounds
Configures this panel as a popup with FloatingTile JSON configuration and bounds
[x, y, width, height]. Call showAsPopup() to display.
Pair with:
  showAsPopup -- display the configured popup
  closeAsPopup -- hide the popup
  setIsModalPopup -- configure modal behavior
Source:
  ScriptingApiContent.cpp  ScriptPanel::setPopupData()
