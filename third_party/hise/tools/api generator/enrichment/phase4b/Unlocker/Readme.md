Unlocker (object)
Obtain via: Engine.createLicenseUnlocker()

RSA key-based product license manager for registration, key file validation,
user identity queries, and expansion unlocking. Built on JUCE OnlineUnlockStatus.
Supports time-limited licenses and optional MuseHub SDK integration.

Common mistakes:
  - Calling loadExpansionList() without HISE_USE_UNLOCKER_FOR_EXPANSIONS enabled --
    throws a script error (compile-time config issue, not recoverable at runtime).
  - Passing expansion key data that doesn't start with "Expansion List" to
    writeExpansionKeyFile() -- silently returns false with no error message.
  - Using Unlocker in frontend without USE_COPY_PROTECTION enabled -- the license
    unlocker singleton does not exist, methods return null/default values.
  - Trusting contains() without checking isUnlocked() first -- contains() returns
    true when unlocker reference is null, bypassing feature checks.

Example:
  // Create a license unlocker reference
  const ul = Engine.createLicenseUnlocker();

  // Check registration status
  if (ul.isUnlocked())
      Console.print("Licensed to: " + ul.getUserEmail());

Methods (16):
  canExpire                checkExpirationData
  checkMuseHub             contains
  getLicenseKeyFile         getRegisteredMachineId
  getUserEmail              isUnlocked
  isValidKeyFile            keyFileExists
  loadExpansionList         loadKeyFile
  setProductCheckFunction   unlockExpansionList
  writeExpansionKeyFile     writeKeyFile
