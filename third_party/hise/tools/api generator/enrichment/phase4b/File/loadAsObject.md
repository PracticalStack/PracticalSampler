File::loadAsObject() -> JSON

Thread safety: UNSAFE -- reads file from disk and parses JSON (I/O).
Reads the file as text and parses it as JSON. Reports a script error with the
JSON parser error message if the content is not valid JSON.

Dispatch/mechanics:
  loadAsString() -> JSON::parse(text, v)
  On parse failure: reportScriptError(r.getErrorMessage())

Pair with:
  writeObject -- to write JSON data to disk
  loadEncryptedObject -- for encrypted JSON persistence

Anti-patterns:
  - Reports a script error on parse failure (unlike loadEncryptedObject which
    silently returns undefined). If the file might not exist or contain invalid
    JSON, check isFile() first or provide a fallback:
    var data = f.loadAsObject(); if (!isDefined(data)) data = {};

Source:
  ScriptingApiObjects.cpp  ScriptFile::loadAsObject()
    -> loadAsString() -> JSON::parse() -> reportScriptError on failure
