ExpansionHandler::setCredentials(var newCredentials) -> undefined

Thread safety: UNSAFE -- stores a var (ref-counted copy) in the core ExpansionHandler.
Sets credentials for decrypting .hxp encrypted expansion packs. Argument must be a
JSON object. Non-object arguments trigger the error function callback (not a script
error), and credentials remain unchanged.
Pair with:
  encodeWithCredentials -- encrypt .hxi files using these credentials
  installExpansionFromPackage -- auto-encrypts during install when credentials are set
  setErrorFunction -- receive notification if credentials are invalid
Anti-patterns:
  - Do NOT pass a string, number, or array -- only JSON objects accepted. Invalid
    types call the error function silently; if no error function is set, the bad
    input is completely ignored
Source:
  ScriptExpansion.cpp:1191  setCredentials()
    -> validates DynamicObject type
    -> ExpansionHandler::setCredentials(var)
