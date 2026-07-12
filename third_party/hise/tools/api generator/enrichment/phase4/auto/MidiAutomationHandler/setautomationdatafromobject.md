Replaces all MIDI automation mappings with the entries from the given array. The array format matches the output of `getAutomationDataObject()`. This is the write half of the read-modify-write cycle for programmatic automation management.

After the data is applied, any registered update callback fires and the MidiLearnPanel refreshes automatically.

> [!Warning:Complete edits before re-reading state] Calling `getAutomationDataObject()` again before writing your changes back returns the old state, not your pending modifications. Always complete your edits on the original snapshot and then call `setAutomationDataFromObject()` once.
