Unlocker::checkMuseHub(Function resultCallback) -> undefined

Thread safety: UNSAFE -- WeakCallbackHolder construction and timer/SDK setup allocate.
Initiates an asynchronous MuseHub license check. The resultCallback is called with
a boolean indicating success. Backend builds simulate with random result after 2s delay.
Callback signature: f(bool ok)

Required setup:
  const var ul = Engine.createLicenseUnlocker();

Dispatch/mechanics:
  Stores callback in WeakCallbackHolder mcheck -> unlocker->checkMuseHub()
  Backend: Timer::callAfterDelay(2000) with 50% random chance, calls mcheck.call1(ok)
  Frontend (HISE_INCLUDE_MUSEHUB): calls checkMuseHubInternal() via real SDK

Pair with:
  isUnlocked -- check unlock status after MuseHub callback succeeds

Anti-patterns:
  - Do NOT use backend results for testing real MuseHub integration -- result is
    random (50% chance) with a fixed 2-second delay.
  - Callback is stored as WeakCallbackHolder -- if the scripting object is destroyed
    before the async result arrives, the callback is silently dropped.

Source:
  ScriptExpansion.cpp  RefObject::checkMuseHub()
    -> mcheck = WeakCallbackHolder(..., resultCallback, 1)
    -> unlocker->checkMuseHub()
  ScriptExpansion.cpp  ScriptUnlocker::checkMuseHub()
    -> USE_BACKEND: Timer::callAfterDelay(2000, random result)
    -> HISE_INCLUDE_MUSEHUB: checkMuseHubInternal()
