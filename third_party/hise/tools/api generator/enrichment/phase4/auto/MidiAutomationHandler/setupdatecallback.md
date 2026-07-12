Registers a callback that fires whenever MIDI automation mappings change - through MIDI learn, the context menu, the MidiLearnPanel, `setAutomationDataFromObject()`, or preset loading. The callback receives a single argument: the complete current automation data array (same format as `getAutomationDataObject()`). The callback also fires once immediately upon registration with the current state.

There is no mechanism to unregister the callback. Passing a non-function value is silently ignored and leaves the previous callback active.

> [!Warning:Never write automation data in callback] Never call `setAutomationDataFromObject()` inside this callback. The write triggers the callback again, creating an infinite loop. Scoped recursion guards do not help because the notification is asynchronous - the guard is already reset by the time the next callback fires.
