Enables or disables exclusive mode for MIDI CC automation. When enabled, each CC number can only control one parameter at a time - assigning a CC to a new parameter removes any existing automation for that CC number. The popup also grays out CC numbers that already have an assignment.

> [!Warning:Only applies to popup and MIDI learn] Exclusive mode only applies to assignments made through the popup and MIDI learn. It does not validate data passed to `setAutomationDataFromObject()`, so you must avoid duplicates in programmatic assignments yourself.
