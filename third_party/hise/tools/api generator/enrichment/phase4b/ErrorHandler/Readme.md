ErrorHandler (object)
Obtain via: Engine.createErrorHandler()

Intercepts system-level error events (missing samples, audio configuration issues,
license problems) and routes them to a scripting callback, replacing HISE's built-in
DeactiveOverlay. Creating an ErrorHandler disables the default overlay permanently --
the script takes full responsibility for error presentation.

Constants:
  Core:
    AppDataDirectoryNotFound = 0       Application data directory missing (broken installation)
  License:
    LicenseNotFound = 1                No license key found (requires HISE_INCLUDE_UNLOCKER_OVERLAY)
    ProductNotMatching = 2             License key is for wrong product/version
    UserNameNotMatching = 3            License user name mismatch
    EmailNotMatching = 4               License email mismatch
    MachineNumbersNotMatching = 5      License machine ID mismatch
    LicenseExpired = 6                 License key has expired
    LicenseInvalid = 7                 License key is invalid
  Custom:
    CriticalCustomErrorMessage = 8     Non-recoverable custom error (terminal state)
    CustomErrorMessage = 12            Custom error message sent by the system
    CustomInformation = 13             Informational message (non-error)
  Samples:
    SamplesNotInstalled = 9            Samples need to be installed from archive
    SamplesNotFound = 10               Sample directory could not be located
  Audio:
    IllegalBufferSize = 11             Audio buffer size is not a valid multiple

Common mistakes:
  - Creating ErrorHandler without calling setErrorCallback() -- disables the default
    overlay with no replacement, making all errors invisible to the user.

Example:
  const eh = Engine.createErrorHandler();

  eh.setErrorCallback(function(state, message)
  {
      // Show custom error UI based on state and message
      Console.print("Error " + state + ": " + message);
  });

Methods (8):
  clearAllErrors              clearErrorLevel
  getCurrentErrorLevel        getErrorMessage
  getNumActiveErrors          setCustomMessageToShow
  setErrorCallback            simulateErrorEvent
