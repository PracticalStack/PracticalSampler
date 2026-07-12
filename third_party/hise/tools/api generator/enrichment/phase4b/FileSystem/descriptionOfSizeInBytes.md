FileSystem::descriptionOfSizeInBytes(Integer bytes) -> String

Thread safety: WARNING -- string construction with atomic ref-count operations
Converts a byte count to a human-readable size string (e.g. "1.5 MB", "200 bytes", "3.2 GB").
Delegates directly to JUCE's File::descriptionOfSizeInBytes() with automatic unit selection.

Pair with:
  getBytesFreeOnVolume -- get byte count to format with this method

Source:
  ScriptingApi.cpp:7394  FileSystem::descriptionOfSizeInBytes()
    -> File::descriptionOfSizeInBytes(bytes)
