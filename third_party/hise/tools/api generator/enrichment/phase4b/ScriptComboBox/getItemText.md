ScriptComboBox::getItemText() -> String

Thread safety: SAFE
Returns the display text of the currently selected item based on the 1-based
value. Returns "" when value is 0, "No options" when value exceeds item count.
Dispatch/mechanics:
  getItemList() -> tokenizes Items property by newline, removes empty strings
    -> if useCustomPopup: filters out headers (**) and separators (___)
    -> indexes with (int)value - 1
    -> if useCustomPopup: strips submenu prefix (text before last "::")
Anti-patterns:
  - When useCustomPopup is enabled, the returned text omits the submenu prefix.
    For "Filters::LowPass", returns "LowPass" not the full string.
Source:
  ScriptingApiContent.cpp:3116  ScriptComboBox::getItemText()
    -> filters headers/separators when useCustomPopup is true
    -> strips submenu prefix via fromLastOccurrenceOf("::")
