# ErrorHandler -- Class Analysis

## Brief
Custom error UI handler replacing HISE's default overlay for system error events.

## Purpose
ErrorHandler intercepts system-level error events (missing samples, audio configuration issues, license problems) and routes them to a scripting callback, replacing HISE's built-in `DeactiveOverlay` component. It maintains a priority-ordered bit field of active error states and always reports the highest-priority (lowest-numbered) active error to the callback. Custom messages can override the default HISE error text per state. Creating an ErrorHandler automatically disables the default overlay, making the script fully responsible for error presentation.

## Details

### Error Priority Model

ErrorHandler tracks active errors as bits in a `BigInteger`. When the callback fires, it always reports the lowest-numbered active error (highest priority). The state enum is ordered by severity:

| Priority | States | Category |
|----------|--------|----------|
| Highest (0) | AppDataDirectoryNotFound | Installation |
| 1-7 | License states (conditional) | Copy protection |
| 8 | CriticalCustomErrorMessage | Custom critical |
| 9-10 | SamplesNotInstalled, SamplesNotFound | Samples |
| 11 | IllegalBufferSize | Audio config |
| 12-13 | CustomErrorMessage, CustomInformation | Custom |

When an error is cleared and others remain, the callback fires again with the next highest-priority error, enabling cascading error resolution UI.

### Message Resolution

See `getErrorMessage()` for full resolution details. In summary, messages resolve in priority order: custom message via `setCustomMessageToShow()`, then event-supplied text, then built-in defaults.

### CriticalCustomErrorMessage is Terminal

Once the system enters `CriticalCustomErrorMessage` state, the `OverlayMessageBroadcaster` rejects all further state changes. This is a non-recoverable error state at the broadcaster level.

### Default Overlay Replacement

Creating an ErrorHandler calls `setUseDefaultOverlay(false)` on the MainController. This disables the built-in `DeactiveOverlay` component. The destructor removes the listener but does NOT re-enable the default overlay. This is a one-way switch for the lifetime of the script session.

### Callback Threading

The error callback arrives on the message thread via JUCE's `AsyncUpdater` with high priority (before regular script callbacks). See `setErrorCallback()` for registration details.

## obtainedVia
`Engine.createErrorHandler()`

## minimalObjectToken
eh

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| AppDataDirectoryNotFound | 0 | int | Application data directory missing (broken installation) | Core |
| LicenseNotFound | 1 | int | No license key found on this machine | License |
| ProductNotMatching | 2 | int | License key is for wrong product/version | License |
| UserNameNotMatching | 3 | int | License user name mismatch | License |
| EmailNotMatching | 4 | int | License email mismatch | License |
| MachineNumbersNotMatching | 5 | int | License machine ID mismatch | License |
| LicenseExpired | 6 | int | License key has expired | License |
| LicenseInvalid | 7 | int | License key is invalid | License |
| CriticalCustomErrorMessage | 8 | int | Non-recoverable custom error (terminal state) | Custom |
| SamplesNotInstalled | 9 | int | Samples need to be installed from archive | Samples |
| SamplesNotFound | 10 | int | Sample directory could not be located | Samples |
| IllegalBufferSize | 11 | int | Audio buffer size is not a valid multiple | Audio |
| CustomErrorMessage | 12 | int | Custom error message sent by the system | Custom |
| CustomInformation | 13 | int | Informational message (non-error) | Custom |

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Creating ErrorHandler without implementing a callback | Creating ErrorHandler and calling `setErrorCallback()` | Creating an ErrorHandler disables the default overlay. Without a callback, errors become invisible to the user. |

## codeExample
```javascript
const eh = Engine.createErrorHandler();

eh.setErrorCallback(function(state, message)
{
    // Show custom error UI based on state and message
    Console.print("Error " + state + ": " + message);
});
```

## Alternatives
Broadcaster -- general-purpose event dispatcher for arbitrary script-level sources, not limited to system error events.

## Related Preprocessors
`HISE_INCLUDE_UNLOCKER_OVERLAY` -- controls whether license-related constants (1-7) are available. Enabled when `USE_COPY_PROTECTION && !USE_SCRIPT_COPY_PROTECTION`.

## Diagnostic Ideas
Reviewed: Yes
Count: 1
- **`error-handler-no-callback`**: Creating an ErrorHandler without ever calling `setErrorCallback()` disables the default overlay with no replacement. Detectable at parse time if `createErrorHandler()` is called but `setErrorCallback` is never invoked on the result.
