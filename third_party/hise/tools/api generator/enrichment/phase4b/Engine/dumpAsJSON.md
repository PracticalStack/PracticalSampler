Engine::dumpAsJSON(var object, String fileName) -> undefined

Thread safety: UNSAFE -- JSON serialization, file path resolution, disk I/O
Exports a JSON object to a file. Relative paths resolve to UserPresets directory.
Anti-patterns:
  - Do NOT pass an Array -- only DynamicObjects accepted. Wrap arrays: {"data": myArray}
Pair with:
  loadFromJSON -- read the file back
Source:
  ScriptingApi.cpp  Engine::dumpAsJSON()
    -> JSON::toString(object, 8) -> File::replaceWithText()
