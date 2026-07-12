Unlocker::setProductCheckFunction(Function checkFunction) -> undefined

Thread safety: UNSAFE -- WeakCallbackHolder construction allocates.
Sets a custom callback that overrides the default product ID matching during key
file validation. By default, HISE strips version numbers from comparisons.
Callback signature: f(String returnedIDFromServer)
The callback must return a boolean indicating whether the product matches.

Required setup:
  const var ul = Engine.createLicenseUnlocker();

Dispatch/mechanics:
  Stores callback in WeakCallbackHolder pcheck
  Called synchronously during loadKeyFile() -> doesProductIDMatch()
    -> pcheck.callSync(&args, 1, &rv) -> returns rv as bool

Pair with:
  loadKeyFile -- the product check callback is invoked during key validation

Anti-patterns:
  - Callback must return a boolean. If it throws or returns a non-boolean, the
    product check falls through and key validation fails silently.

Source:
  ScriptExpansion.cpp  RefObject::setProductCheckFunction()
    -> pcheck = WeakCallbackHolder(..., checkFunction, 1)
  ScriptExpansion.cpp  ScriptUnlocker::doesProductIDMatch()
    -> currentObject->pcheck.callSync(&args, 1, &rv)
