Download::getDownloadedTarget() -> ScriptObject

Thread safety: UNSAFE -- creates a new ScriptFile object (heap allocation via new ScriptFile).
Returns the target file as a File scripting object. Always returns the path regardless of
download state -- the file may not exist yet, be partially written, or have been deleted.

Anti-patterns:
  - Do NOT assume the returned file exists or is complete -- check data.finished and
    data.success before reading or moving the file.

Source:
  ScriptingApiObjects.cpp:~1290  ScriptDownloadObject::getDownloadedTarget()
    -> new ScriptFile(targetFile) unconditionally
