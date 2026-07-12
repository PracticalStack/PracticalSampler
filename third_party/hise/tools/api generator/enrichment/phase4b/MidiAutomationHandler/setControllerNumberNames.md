MidiAutomationHandler::setControllerNumberNames(String ccName, Array nameArray) -> undefined

Thread safety: UNSAFE -- constructs a StringArray from the input, involving String allocation.
Customizes the display names in the right-click MIDI automation popup. ccName replaces
the default "MIDI CC" section header. nameArray provides per-CC display names (index 0 =
CC#0, index 1 = CC#1, etc.). CCs beyond the array length fall back to "CC#N".

Required setup:
  const var mah = Engine.createMidiAutomationHandler();
  mah.setControllerNumbersInPopup([1, 11]);

Dispatch/mechanics:
  setCCName(ccName) -> replaces "MIDI CC" label in popup header
  setControllerPopupNames(StringArray) -> getControllerName(i) returns custom name
    at index i, or falls back to "CC#" + i

Pair with:
  setControllerNumbersInPopup -- restrict which CCs appear; always pair both calls
    so filtered CCs have readable names instead of raw "CC#N" labels

Source:
  ScriptingApiObjects.cpp:10074  ScriptedMidiAutomationHandler::setControllerNumberNames()
    -> handler->setCCName(ccName)
    -> handler->setControllerPopupNames(sa)
  MacroControlledComponents.cpp:~145  addAutomationMenuItems()
    -> uses getCCName() for section header, getControllerName(i) for CC labels
