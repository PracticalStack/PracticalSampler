Sets credentials for decrypting `.hxp` encrypted expansion packs. The argument must be a JSON object. After setting credentials, encrypted expansions that were previously uninitialised will attempt to load on the next `refreshExpansions()` call (or automatically if credentials are entered through the standard flow).

> [!Warning:Non-object input fails silently] Passing a string, number, or array does not throw a script error. Instead, it sends a message via the error function callback. If no error function is set, the invalid input is silently ignored and credentials remain unchanged.
