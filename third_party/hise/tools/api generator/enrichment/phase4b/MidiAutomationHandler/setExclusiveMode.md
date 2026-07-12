MidiAutomationHandler::setExclusiveMode(Integer shouldBeExclusive) -> undefined

Thread safety: SAFE
Enables or disables exclusive mode for MIDI CC automation. When active, each CC number
can only control one parameter. Assigning a CC to a new parameter first removes all
existing automations for that CC. The popup also grays out already-assigned CCs.

Dispatch/mechanics:
  Sets the exclusiveMode bool flag on MidiControllerAutomationHandler.
  Two runtime effects:
    1. MIDI learn (setUnlearnedMidiControlNumber): clears existing entries for the
       CC before adding the new assignment
    2. Popup (isMappable): CC is only mappable if no entries exist for it

Pair with:
  setControllerNumbersInPopup -- exclusive mode affects which CCs are shown as available

Source:
  MainControllerHelpers.h:109   MidiControllerAutomationHandler::setExclusiveMode()
    -> simple bool flag assignment
  MainControllerHelpers.cpp      setUnlearnedMidiControlNumber()
    -> if exclusiveMode: removes all entries for the CC key before adding new one
  MacroControlledComponents.cpp  isMappable()
    -> if exclusiveMode: returns false for CCs with existing automations
