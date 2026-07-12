## clearAllErrors

**Signature:** `undefined clearAllErrors()`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.clearAllErrors();`

**Description:**
Clears all active error states at once. After this call, `getCurrentErrorLevel()` returns -1 and `getNumActiveErrors()` returns 0. Does not fire the error callback.

**Parameters:**
None.

**Pitfalls:**
- The error callback does not fire after clearing. When using clearAllErrors to dismiss errors, the script must explicitly update its error UI (e.g., hide the overlay) rather than relying on the callback to report the cleared state.

**Cross References:**
- `$API.ErrorHandler.clearErrorLevel$`
- `$API.ErrorHandler.getCurrentErrorLevel$`

## clearErrorLevel

**Signature:** `undefined clearErrorLevel(int stateToClear)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Triggers the error callback synchronously when other errors remain, involving String construction via getErrorMessage().
**Minimal Example:** `{obj}.clearErrorLevel(eh.SamplesNotFound);`

**Description:**
Clears a single error state by unsetting its bit. If other errors remain active after clearing, the error callback fires immediately with the next highest-priority (lowest-numbered) active error, enabling cascading error resolution. If no errors remain after clearing, the callback does not fire.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| stateToClear | Integer | no | The error state constant to clear | Valid ErrorHandler state constant (0-13) |

**Pitfalls:**
- When the last active error is cleared, the callback does not fire. The script must handle the "all clear" transition explicitly after calling clearErrorLevel, for example by checking `getNumActiveErrors() == 0`.

**Cross References:**
- `$API.ErrorHandler.clearAllErrors$`
- `$API.ErrorHandler.getCurrentErrorLevel$`
- `$API.ErrorHandler.setErrorCallback$`

## getCurrentErrorLevel

**Signature:** `int getCurrentErrorLevel()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var level = {obj}.getCurrentErrorLevel();`

**Description:**
Returns the state constant of the highest-priority (lowest-numbered) active error, or -1 if no errors are active. The priority follows the state enum order: lower numbers represent higher priority.

**Parameters:**
None.

**Pitfalls:**
- Returns the lowest-numbered active state, not the most recently received one. If both SamplesNotFound (10) and IllegalBufferSize (11) are active, this returns 10 regardless of which arrived last.

**Cross References:**
- `$API.ErrorHandler.getErrorMessage$`
- `$API.ErrorHandler.getNumActiveErrors$`

## getErrorMessage

**Signature:** `String getErrorMessage()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String return type involves atomic ref-count operations.
**Minimal Example:** `var msg = {obj}.getErrorMessage();`

**Description:**
Returns the error message for the current highest-priority active error. Message resolution follows a priority chain: (1) custom message set via `setCustomMessageToShow()` for that state, (2) message text received with the error event (for CustomErrorMessage, CriticalCustomErrorMessage, and CustomInformation states), (3) the built-in default HISE message. Returns an empty string if no errors are active.

**Parameters:**
None.

**Cross References:**
- `$API.ErrorHandler.getCurrentErrorLevel$`
- `$API.ErrorHandler.setCustomMessageToShow$`
- `$API.ErrorHandler.setErrorCallback$`

## getNumActiveErrors

**Signature:** `int getNumActiveErrors()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var count = {obj}.getNumActiveErrors();`

**Description:**
Returns the number of currently active error states. Returns 0 when no errors are active.

**Parameters:**
None.

**Cross References:**
- `$API.ErrorHandler.getCurrentErrorLevel$`
- `$API.ErrorHandler.clearAllErrors$`

## setCustomMessageToShow

**Signature:** `undefined setCustomMessageToShow(int state, String messageToShow)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** String copy into internal StringArray involves heap allocation.
**Minimal Example:** `{obj}.setCustomMessageToShow(eh.SamplesNotFound, "Please locate the samples folder.");`

**Description:**
Overrides the default error message for a specific state. The custom message takes highest priority when `getErrorMessage()` resolves text for that state, above both event-supplied messages and built-in defaults. Works for all states, not only the custom message states (CustomErrorMessage, CriticalCustomErrorMessage, CustomInformation).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| state | Integer | no | The error state constant to customize | Valid ErrorHandler state constant (0-13) |
| messageToShow | String | no | The custom message text to display for this state | -- |

**Pitfalls:**
- Custom messages persist across error cycles. If a state is cleared and later re-triggered, the custom message still applies. Pass an empty string to revert to the default message for that state.

**Cross References:**
- `$API.ErrorHandler.getErrorMessage$`
- `$API.ErrorHandler.setErrorCallback$`

## setErrorCallback

**Signature:** `undefined setErrorCallback(var errorCallback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a WeakCallbackHolder with heap allocation.
**Minimal Example:** `{obj}.setErrorCallback(onError);`

**Description:**
Registers a callback function that fires when an error state changes. The callback receives two arguments: the state constant of the highest-priority active error and the resolved error message string. The callback fires on the message thread with high priority (before regular script callbacks). Replaces any previously registered callback.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| errorCallback | Function | no | The callback to invoke on error state changes | Must be a JavaScript function |

**Callback Signature:** errorCallback(state: int, message: String)

**Pitfalls:**
- [BUG] Passing a non-function value (including `false`) silently does nothing -- it does not clear the existing callback. There is no way to unregister a callback once registered.

**Cross References:**
- `$API.ErrorHandler.simulateErrorEvent$`
- `$API.ErrorHandler.getCurrentErrorLevel$`
- `$API.ErrorHandler.getErrorMessage$`
- `$API.ErrorHandler.setCustomMessageToShow$`

**Example:**
```javascript:error-callback-setup
// Title: Registering a custom error callback
const var eh = Engine.createErrorHandler();

reg lastState = -1;

inline function onError(state, message)
{
    lastState = state;
};

eh.setErrorCallback(onError);

// --- test-only ---
eh.simulateErrorEvent(eh.IllegalBufferSize);
// --- end test-only ---
```
```json:testMetadata:error-callback-setup
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "lastState", "value": 11}
  ]
}
```

## simulateErrorEvent

**Signature:** `undefined simulateErrorEvent(int state)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Triggers the error callback synchronously, involving String construction via getErrorMessage().
**Minimal Example:** `{obj}.simulateErrorEvent(eh.SamplesNotFound);`

**Description:**
Simulates an error event as if it came from the HISE system. Sets the specified state as active and fires the error callback with the highest-priority active error. Useful for testing error handling UI during development without triggering real system errors.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| state | Integer | no | The error state constant to simulate | Valid ErrorHandler state constant (0-13) |

**Cross References:**
- `$API.ErrorHandler.setErrorCallback$`
- `$API.ErrorHandler.clearErrorLevel$`
