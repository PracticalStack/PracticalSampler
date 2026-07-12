Unlocker::loadExpansionList() -> Integer

Thread safety: UNSAFE -- File I/O, RSA decryption, BlowFish decryption, XML parsing.
Loads and decrypts the expansion list file, then passes credentials to
ExpansionHandler.setCredentials() to unlock individual expansions. Requires
HISE_USE_UNLOCKER_FOR_EXPANSIONS and isUnlocked() == true.

Dispatch/mechanics:
  Read expansion file -> strip header -> hex to BigInteger -> RSA decrypt
    -> BlowFish decrypt (keyed with machine ID) -> parse XML payload
    -> validate email/machineId/product -> ExpansionHandler.setCredentials()
  Auto-called by constructor if preprocessor is enabled and plugin is unlocked.

Pair with:
  writeExpansionKeyFile -- write expansion data, which then calls loadExpansionList
  unlockExpansionList -- backend-only alternative for development
  isUnlocked -- must be unlocked first; returns false otherwise

Anti-patterns:
  - Do NOT call without HISE_USE_UNLOCKER_FOR_EXPANSIONS enabled -- throws a script
    error (compile-time config issue, not recoverable at runtime).

Source:
  ScriptExpansion.cpp  RefObject::loadExpansionList()
    -> HISE_GET_PREPROCESSOR check -> isUnlocked() check
    -> ExpansionHandler.setCredentials(unlocker->getExpansionList())
  ScriptExpansion.cpp  ScriptUnlocker::getExpansionList()
    -> RSA + BlowFish double decryption -> XML parse -> DynamicObject with slug/key pairs
