Clears all active error states at once. After this call, `getCurrentErrorLevel()` returns -1 and `getNumActiveErrors()` returns 0.

> [!Warning:Callback does not fire after clearing] The error callback does not fire when all errors are cleared. Update your error UI explicitly after calling this method rather than relying on the callback to report the cleared state.