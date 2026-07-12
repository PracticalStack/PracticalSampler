Engine::isControllerUsedByAutomation(var controllerNumber) -> Integer

Thread safety: SAFE
Checks if a MIDI CC is assigned to parameter automation. Returns automation index
or -1. Accepts int (any channel) or [channel, ccNumber] array.
Source:
  ScriptingApi.cpp  Engine::isControllerUsedByAutomation()
    -> MidiControlAutomationHandler::getCCNumberForAutomationIndex()
