ScriptPanel::setIsModalPopup(Integer shouldBeModal) -> undefined

Thread safety: SAFE
Sets whether this popup panel displays with a dark modal background overlay.
When modal, clicks outside the popup close it. Only relevant for panels used
as popups via showAsPopup().
Pair with:
  showAsPopup -- show the popup after configuring modality
  closeAsPopup -- close the popup
Source:
  ScriptingApiContent.cpp  ScriptPanel::setIsModalPopup()
