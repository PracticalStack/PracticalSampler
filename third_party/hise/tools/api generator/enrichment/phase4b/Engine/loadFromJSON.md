Engine::loadFromJSON(String fileName) -> Object

Thread safety: UNSAFE -- file I/O, JSON parsing (heap allocations)
Reads a JSON file and returns the parsed object. Relative paths resolve to UserPresets
directory. Returns undefined (not error) if file does not exist.
Anti-patterns:
  - Missing files return undefined silently -- always check with isDefined()
  - Relative paths resolve to UserPresets directory, not project root
Pair with:
  dumpAsJSON -- write JSON to file
Source:
  ScriptingApi.cpp  Engine::loadFromJSON()
    -> File::loadFileAsString() -> JSON::parse()
