ExpansionHandler::encodeWithCredentials(var hxiFile) -> bool

Thread safety: UNSAFE -- File I/O and BlowFish encryption operations.
Encrypts an intermediate .hxi file into a credential-encrypted .hxp file using
credentials previously set via setCredentials().
Required setup:
  const var eh = Engine.createExpansionHandler();
  eh.setCredentials({"key": "myLicenseKey"});
Dispatch/mechanics:
  ScriptEncryptedExpansion::encryptIntermediateFile(hxiFile, credentials)
    -> BlowFish encryption of ValueTree data -> writes .hxp file
Pair with:
  setCredentials -- must set credentials before encoding
Anti-patterns:
  - Do NOT call without setting credentials first -- encryption may fail or
    produce an invalid .hxp file
Source:
  ScriptExpansion.cpp:1307  encodeWithCredentials()
    -> ScriptEncryptedExpansion::encryptIntermediateFile()
