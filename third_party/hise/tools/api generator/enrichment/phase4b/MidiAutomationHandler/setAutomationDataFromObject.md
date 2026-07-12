MidiAutomationHandler::setAutomationDataFromObject(Array automationData) -> undefined

Thread safety: UNSAFE -- rebuilds all automation entries from ValueTree. Involves heap allocation, String construction, and change notification dispatch.
Replaces all MIDI automation mappings with the entries from the given array. Format
matches getAutomationDataObject() output. Clears all existing entries before adding new
ones. Fires the update callback after restoring.

Required setup:
  const var mah = Engine.createMidiAutomationHandler();
  var data = mah.getAutomationDataObject();

Dispatch/mechanics:
  convertVarArrayToFlatValueTree(automationData, "MidiAutomation", "Controller")
    -> restoreFromValueTree() clears all entries, rebuilds from ValueTree
    -> sendSynchronousChangeMessage() during preset loads, sendChangeMessage() otherwise
    -> triggers registered update callback with fresh data snapshot

Pair with:
  getAutomationDataObject -- read current state before modifying
  setUpdateCallback -- observe the resulting change notification

Anti-patterns:
  - Do NOT pass a non-Array value (single object, number, string) -- silently clears
    all automation data. The internal converter produces an empty ValueTree and
    restoreFromValueTree wipes everything.
  - Do NOT call from inside the update callback -- causes infinite recursion during
    synchronous preset loads (restoreFromValueTree fires sendSynchronousChangeMessage).

Source:
  ScriptingApiObjects.cpp:10074  ScriptedMidiAutomationHandler
  MainControllerHelpers.cpp      restoreFromValueTree()
    -> clear() removes all entries
    -> iterates ValueTree children, creates AutomationData per "Controller" child
    -> resolves Attribute field (string ID or numeric index)
    -> sendSynchronousChangeMessage() or sendChangeMessage()
