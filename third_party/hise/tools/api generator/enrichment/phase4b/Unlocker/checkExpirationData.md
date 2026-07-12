Unlocker::checkExpirationData(String encodedTimeString) -> NotUndefined

Thread safety: UNSAFE -- RSA decryption, ISO8601 parsing, and string operations allocate.
Validates an RSA-encrypted expiration timestamp and unlocks the plugin for the
encoded duration. Returns days remaining (Integer) on success, false on RSA failure,
or a String error message on format/null errors.

Required setup:
  const var ul = Engine.createLicenseUnlocker();

Dispatch/mechanics:
  hex string -> BigInteger::parseString(hex) -> RSA applyToValue(bi)
    -> toMemoryBlock().toString() -> Time::fromISO8601(timeString)
    -> unlockWithTime(time) -> roundToInt(delta.inDays())
  In frontend builds, successful call also triggers sample reloading.

Pair with:
  canExpire -- check if license supports expiration before calling

Anti-patterns:
  - Do NOT check return value by truthiness alone -- a String error message
    like "encodedTimeString data is corrupt" is truthy. Check typeof result == "number".
  - Input must start with "0x" or returns the string "encodedTimeString data is corrupt".

Source:
  ScriptExpansion.cpp  RefObject::checkExpirationData()
    -> BigInteger parseString(hex) -> RSA applyToValue
    -> Time::fromISO8601 -> unlockWithTime(time)
    -> returns roundToInt(delta.inDays())
