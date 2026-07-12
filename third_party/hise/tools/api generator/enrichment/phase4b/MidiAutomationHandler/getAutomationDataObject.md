MidiAutomationHandler::getAutomationDataObject() -> Array

Thread safety: UNSAFE -- allocates DynamicObjects and Array elements when converting internal ValueTree to JSON.
Returns the complete MIDI automation configuration as an array of JSON objects.
Each object represents one CC-to-parameter mapping. The returned array is a snapshot --
modifying it does not affect live data. Use setAutomationDataFromObject() to write back.

Required setup:
  const var mah = Engine.createMidiAutomationHandler();

Dispatch/mechanics:
  exportAsValueTree() on MidiControllerAutomationHandler
    -> ValueTreeConverters::convertFlatValueTreeToVarArray()
    -> flat property copy: each ValueTree child becomes a JSON object

Pair with:
  setAutomationDataFromObject -- write modified data back (round-trip)
  setUpdateCallback -- observe changes reactively instead of polling

Source:
  ScriptingApiObjects.cpp:10074  ScriptedMidiAutomationHandler constructor
  MainControllerHelpers.h:109   MidiControllerAutomationHandler::exportAsValueTree()
    -> iterates Container<AutomationData> via createIterator()
    -> each AutomationData serializes to ValueTree child with tag "Controller"
    -> convertFlatValueTreeToVarArray() produces JSON array
