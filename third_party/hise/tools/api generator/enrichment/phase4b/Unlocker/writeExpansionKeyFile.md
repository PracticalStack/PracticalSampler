Unlocker::writeExpansionKeyFile(String keyData) -> Integer

Thread safety: UNSAFE -- File I/O write, then delegates to loadExpansionList() for decryption.
Writes expansion key data to the expansion list file (sibling of main license key
file named "expansions" with same extension). On successful write, automatically
calls loadExpansionList() to decrypt and apply expansion credentials.

Required setup:
  const var ul = Engine.createLicenseUnlocker();
  // HISE_USE_UNLOCKER_FOR_EXPANSIONS must be enabled

Dispatch/mechanics:
  Validates keyData starts with "Expansion List" -> writes to getExpansionListFile()
    -> calls loadExpansionList() which decrypts and applies credentials

Pair with:
  loadExpansionList -- auto-called after successful write
  unlockExpansionList -- backend-only alternative for development
  getLicenseKeyFile -- expansion file is a sibling of this path

Anti-patterns:
  - [BUG] Silently returns false if keyData doesn't start with "Expansion List" --
    no error message produced. Validate input format before calling.
  - Throws a script error if HISE_USE_UNLOCKER_FOR_EXPANSIONS is not enabled.

Source:
  ScriptExpansion.cpp  RefObject::writeExpansionKeyFile()
    -> HISE_GET_PREPROCESSOR check -> startsWith("Expansion List") check
    -> write to getExpansionListFile() -> loadExpansionList()
