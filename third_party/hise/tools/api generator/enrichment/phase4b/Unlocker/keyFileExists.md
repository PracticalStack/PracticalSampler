Unlocker::keyFileExists() -> Integer

Thread safety: UNSAFE -- File system existence check performs I/O.
Returns whether the license key file exists on disk at the expected location
(path from getLicenseKeyFile()). Use to decide whether to show a registration
dialog or attempt to load an existing key.

Pair with:
  getLicenseKeyFile -- returns the path being checked
  loadKeyFile -- load and validate the key if it exists

Source:
  ScriptExpansion.cpp  RefObject::keyFileExists()
    -> getLicenseKeyFile().existsAsFile()
