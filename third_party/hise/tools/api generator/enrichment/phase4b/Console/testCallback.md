Console::testCallback(ScriptObject obj, String callbackId, NotUndefined argList) -> undefined

Thread safety: SAFE
Synchronously invokes a named callback on a UI component for automated testing. `obj` must be a ScriptComponent reference, `callbackId` identifies which callback to trigger, `argList` is a single value or array of arguments. Prints diagnostic messages (BEGIN_CALLBACK_TEST, END_CALLBACK_TEST, CALLBACK_ARGS) to the console. Reports a script error if callback execution fails.

Anti-patterns:
- Intended only for automated testing. A warning is logged if called outside a testing configuration (`isFlakyThreadingAllowed` is false).
- The component must support the specified callback; otherwise the internal `testCallback` returns an error.
