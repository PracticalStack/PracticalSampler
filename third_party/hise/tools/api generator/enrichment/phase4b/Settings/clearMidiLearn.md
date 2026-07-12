Settings::clearMidiLearn() -> undefined

Thread safety: UNSAFE -- calls MidiControlAutomationHandler::clear() with sendNotification, triggers listener updates and potential UI repaints
Removes all MIDI controller-to-parameter mappings from the MIDI automation handler.
A notification is sent to update the UI.

Dispatch/mechanics:
  mc->getMacroManager().getMidiControlAutomationHandler()->clear(sendNotification)

Source:
  ScriptingApi.cpp  Settings::clearMidiLearn()
    -> MidiControlAutomationHandler::clear(sendNotification)
