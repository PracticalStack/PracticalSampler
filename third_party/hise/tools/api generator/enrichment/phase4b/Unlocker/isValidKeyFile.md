Unlocker::isValidKeyFile(String possibleKeyData) -> Integer

Thread safety: WARNING -- String involvement, atomic ref-count operations.
Checks whether the given string looks like a valid JUCE RSA key file by verifying
it starts with "Keyfile for ". Format check only -- does NOT perform RSA signature
validation. Use to pre-validate user-provided key data before writeKeyFile().

Pair with:
  writeKeyFile -- validate data with isValidKeyFile before writing
  loadKeyFile -- performs actual RSA validation after writing

Source:
  ScriptExpansion.cpp  RefObject::isValidKeyFile()
    -> possibleKeyData.toString().startsWith("Keyfile for ")
