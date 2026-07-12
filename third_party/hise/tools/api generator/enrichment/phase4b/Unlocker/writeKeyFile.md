Unlocker::writeKeyFile(String keyData) -> Integer

Thread safety: UNSAFE -- File I/O write to the license key file location.
Writes key file content to the license key file location on disk. Does NOT
validate or load the key -- call loadKeyFile() afterwards to validate the RSA
signature and unlock the plugin.

Required setup:
  const var ul = Engine.createLicenseUnlocker();

Pair with:
  isValidKeyFile -- pre-validate data format before writing
  loadKeyFile -- validate and activate the key after writing
  getLicenseKeyFile -- the path where this method writes

Source:
  ScriptExpansion.cpp  RefObject::writeKeyFile()
    -> writes keyData to getLicenseKeyFile() path
