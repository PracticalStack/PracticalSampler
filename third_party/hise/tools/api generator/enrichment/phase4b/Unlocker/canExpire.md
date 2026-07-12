Unlocker::canExpire() -> Integer

Thread safety: SAFE
Returns whether the current license has an expiration time set. Checks if
JUCE OnlineUnlockStatus::getExpiryTime() is non-zero.

Dispatch/mechanics:
  unlocker->getExpiryTime() != juce::Time(0)
  Returns false (not error) when unlocker reference is null.

Pair with:
  checkExpirationData -- validate and extend an expiration-based license

Anti-patterns:
  - Do NOT assume false means "no expiration" -- it could also mean the unlocker
    reference is null (singleton not created). Guard with isUnlocked() first.

Source:
  ScriptExpansion.cpp  RefObject::canExpire()
    -> unlocker->getExpiryTime() != juce::Time(0)
