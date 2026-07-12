UserPresetHandler::createObjectForAutomationValues() -> Array

Thread safety: WARNING -- allocates new DynamicObject and Array for each automation slot
Returns an array of objects representing current values of all custom automation
slots. Each element has "id" (String) and "value" (Double). Output can be passed
to updateAutomationValues to restore automation state.
Required setup:
  const var uph = Engine.createUserPresetHandler();
  // Custom automation must be set up first
Pair with:
  updateAutomationValues -- restore values from the returned snapshot
  setCustomAutomation -- must define automation slots first
Source:
  ScriptExpansion.cpp  createObjectForAutomationValues()
    -> iterates CustomAutomationData list
    -> creates {id, value} object per slot
