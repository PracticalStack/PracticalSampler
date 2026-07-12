<!-- Diagram triage:
  - No diagrams specified in Phase 1 data
-->

# ErrorHandler

ErrorHandler replaces HISE's built-in error overlay with a scriptable callback, giving you full control over how system errors are presented to the user. The default overlay is a recognisable HISE fingerprint - implementing a custom error handler lets you present errors in a style consistent with your plugin's UI.

The class intercepts several categories of system error:

- **Installation errors** - missing application data directory
- **License errors** - missing, expired, or mismatched license keys
- **Sample errors** - samples not installed or not found
- **Audio configuration errors** - illegal buffer size
- **Custom messages** - error and informational messages triggered by the system or by `Engine.showErrorMessage()`

Each error state has a numeric priority. When multiple errors are active simultaneously, ErrorHandler always reports the highest-priority (lowest-numbered) state to the callback. Clearing one error automatically surfaces the next in the queue.

```
const eh = Engine.createErrorHandler();
```

The error states are ordered by severity:

| Constant | Value | Category |
|----------|-------|----------|
| `ErrorHandler.AppDataDirectoryNotFound` | 0 | Installation |
| `ErrorHandler.LicenseNotFound` | 1 | License |
| `ErrorHandler.ProductNotMatching` | 2 | License |
| `ErrorHandler.UserNameNotMatching` | 3 | License |
| `ErrorHandler.EmailNotMatching` | 4 | License |
| `ErrorHandler.MachineNumbersNotMatching` | 5 | License |
| `ErrorHandler.LicenseExpired` | 6 | License |
| `ErrorHandler.LicenseInvalid` | 7 | License |
| `ErrorHandler.CriticalCustomErrorMessage` | 8 | Custom (terminal) |
| `ErrorHandler.SamplesNotInstalled` | 9 | Samples |
| `ErrorHandler.SamplesNotFound` | 10 | Samples |
| `ErrorHandler.IllegalBufferSize` | 11 | Audio |
| `ErrorHandler.CustomErrorMessage` | 12 | Custom |
| `ErrorHandler.CustomInformation` | 13 | Custom |

> Creating an ErrorHandler automatically disables the default HISE overlay - this is a one-way switch. The script becomes fully responsible for error presentation from that point on, so always register a callback immediately after creation.

> `CriticalCustomErrorMessage` is a terminal state. Once the system enters this state, all further state changes are rejected. This is non-recoverable for the lifetime of the session.

## Common Mistakes

- **Always register a callback after creation**
  **Wrong:** Creating an ErrorHandler without calling `setErrorCallback()`
  **Right:** Call `setErrorCallback()` immediately after `Engine.createErrorHandler()`
  *Creating an ErrorHandler disables the default overlay. Without a callback, errors become invisible to the user - the overlay is gone but nothing replaces it.*

- **Handle the "all clear" state explicitly**
  **Wrong:** Relying on the error callback to hide your error UI when the last error is cleared
  **Right:** Check `getNumActiveErrors() == 0` after calling `clearErrorLevel()` or `clearAllErrors()`
  *The callback does not fire when transitioning from one active error to zero errors. Your script must detect this state and dismiss the error UI manually.*
