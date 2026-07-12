FileSystem::getBytesFreeOnVolume(var folder) -> Integer

Thread safety: UNSAFE -- OS filesystem query via JUCE's File::getBytesFreeOnVolume()
Returns the number of free bytes on the volume containing the specified folder. Accepts
a SpecialLocations constant (int) or a File object. Does not accept path strings.

Pair with:
  descriptionOfSizeInBytes -- format the returned byte count for display
  getFolder -- convert SpecialLocations constant to File if needed

Anti-patterns:
  - Do NOT pass a path string -- returns 0 silently (indistinguishable from a full volume).
    Use fromAbsolutePath() first to get a File object.

Source:
  ScriptingApi.cpp:7394  FileSystem::getBytesFreeOnVolume()
    -> resolves folder (int constant or ScriptFile) -> File::getBytesFreeOnVolume()
