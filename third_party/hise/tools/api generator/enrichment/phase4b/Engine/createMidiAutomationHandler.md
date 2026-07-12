Engine::createMidiAutomationHandler() -> ScriptObject

Thread safety: UNSAFE -- heap allocation
Creates a MIDI automation handler for script-level access to MIDI CC-to-parameter
mappings. Allows programmatic configuration of MIDI learn assignments, automation
slots, and MPE data handling.
Pair with:
  isControllerUsedByAutomation -- check if a CC is assigned to automation
  createUserPresetHandler -- automation state is part of preset system
Source:
  ScriptingApi.cpp  Engine::createMidiAutomationHandler()
    -> new ScriptedMidiAutomationHandler wrapping MainController's automation handler
