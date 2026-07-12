Unlocker::loadKeyFile() -> Integer

Thread safety: UNSAFE -- File I/O, RSA validation, string parsing.
Reads the license key file from disk and validates it via RSA signature verification.
On success, the plugin becomes unlocked; machine ID and email become available via
getRegisteredMachineId() and getUserEmail(). Auto-called by constructor if key file exists.

Dispatch/mechanics:
  Read file from getLicenseKeyFile() path -> parse "Machine numbers: " line
    -> applyKeyFile(keyData) (JUCE RSA validation)
    -> doesProductIDMatch() virtual (strips version by default, or uses custom callback)

Pair with:
  writeKeyFile -- write key data to disk, then call loadKeyFile to validate
  setProductCheckFunction -- override default version-stripping product ID match
  isUnlocked -- check result after loading

Anti-patterns:
  - Do NOT assume version-specific key validation -- by default "MyPlugin 1.0" key
    validates "MyPlugin 2.0". Use setProductCheckFunction() for version-aware matching.

Source:
  ScriptExpansion.cpp  RefObject::loadKeyFile()
    -> reads getLicenseKeyFile() -> parses machine ID
    -> juce::OnlineUnlockStatus::applyKeyFile(keyData)
  ScriptExpansion.cpp  ScriptUnlocker::doesProductIDMatch()
    -> custom pcheck callback if set, otherwise strips version and compares names
