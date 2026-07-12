MidiAutomationHandler::setConsumeAutomatedControllers(Integer shouldBeConsumed) -> undefined

Thread safety: SAFE
Controls whether MIDI CC messages matching an automation entry are removed from the
MIDI buffer before reaching script callbacks. When enabled (the default), automated CC
messages never arrive in onController. When disabled, they pass through.

Dispatch/mechanics:
  Sets the consumeEvents bool flag on MidiControllerAutomationHandler.
  At runtime, handleControllerMessage() returns this flag when a CC matches
    -> handleParameterData() uses the return value to include or exclude the
       message from the output MidiBuffer.

Anti-patterns:
  - Do NOT disable consumption expecting automated CCs to stop setting parameters --
    the parameter is always set regardless of this flag. This only controls whether
    the CC message also reaches script MIDI callbacks.

Source:
  ScriptingApiObjects.cpp:10074  ScriptedMidiAutomationHandler
  MainControllerHelpers.cpp:943  handleControllerMessage()
    -> returns consumeEvents flag after processing automation
    -> caller: handleParameterData() uses flag to filter MidiBuffer
