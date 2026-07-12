MidiAutomationHandler::setControllerNumbersInPopup(Array numberArray) -> undefined

Thread safety: UNSAFE -- constructs a BigInteger bitmask from the array, which may involve heap allocation.
Restricts which CC numbers appear in the right-click MIDI automation popup on UI
components. When a filter is set, the popup changes from a nested submenu with "Learn"
to a flat list of the specified CCs. Passing an empty array resets to default (all 128
CCs in a submenu).

Required setup:
  const var mah = Engine.createMidiAutomationHandler();

Dispatch/mechanics:
  Iterates numberArray, sets each CC number as a bit in a BigInteger bitmask
    -> handler->setControllerPopupNumbers(bitmask)
  Popup checks shouldAddControllerToPopup(i): true if bitmask is zero (all CCs)
    or if bit i is set
  hasSelectedControllerPopupNumbers() controls flat-list vs submenu layout

Pair with:
  setControllerNumberNames -- always pair so filtered CCs have readable display names
  setExclusiveMode -- in exclusive mode, popup additionally grays out already-assigned CCs

Source:
  ScriptingApiObjects.cpp:10074  ScriptedMidiAutomationHandler::setControllerNumbersInPopup()
    -> BigInteger bitmask from array -> handler->setControllerPopupNumbers(bi)
  MacroControlledComponents.cpp:~145  addAutomationMenuItems()
    -> hasSelectedControllerPopupNumbers() ? flat list : submenu with Learn
