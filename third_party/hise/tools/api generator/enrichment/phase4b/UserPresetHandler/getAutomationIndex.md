UserPresetHandler::getAutomationIndex(String automationID) -> Integer

Thread safety: SAFE
Returns the zero-based index of a custom automation slot by its string ID.
Returns -1 if the custom data model is not active or the ID is not found.
Pair with:
  setAutomationValue -- use the returned index to set values
  setCustomAutomation -- defines the automation slots and their indices
Source:
  ScriptExpansion.cpp  getAutomationIndex()
    -> looks up automationID in CustomAutomationData list
    -> returns position index or -1
