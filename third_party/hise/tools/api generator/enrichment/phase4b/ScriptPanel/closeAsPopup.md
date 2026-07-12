ScriptPanel::closeAsPopup() -> undefined

Thread safety: UNSAFE -- modifies popup panel list and visibility state
Hides this panel if currently shown as a popup overlay. Sets shownAsPopup to false
and removes from Content's popup panel list. No effect if not shown as popup.
Pair with:
  showAsPopup -- show the popup
  isVisibleAsPopup -- check popup visibility state
  setIsModalPopup -- configure modal behavior before showing
Source:
  ScriptingApiContent.cpp  ScriptPanel::closeAsPopup()
