Returns the error message string for the current highest-priority active error. Returns an empty string if no errors are active.

Messages resolve in priority order:

1. Custom message set via `setCustomMessageToShow()` for that state
2. Text supplied with the error event (for custom states, including messages from `Engine.showErrorMessage()`)
3. Built-in HISE default message for that state