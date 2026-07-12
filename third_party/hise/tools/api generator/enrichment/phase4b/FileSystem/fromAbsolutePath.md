FileSystem::fromAbsolutePath(String path) -> ScriptObject

Thread safety: UNSAFE -- allocates ScriptFile object on the heap, string construction
Creates a File object from an absolute filesystem path string. Returns undefined if the
string is not recognized as an absolute path by JUCE's File::isAbsolutePath(). Does not
verify file/directory existence -- use File.isFile() or File.isDirectory() to check.

Pair with:
  getFolder -- for well-known locations via SpecialLocations constants
  fromReferenceString -- for HISE {PROJECT_FOLDER} reference strings
  browse -- fromAbsolutePath can reconstruct a File from a stored path for use as startFolder

Anti-patterns:
  - Do NOT pass relative paths -- returns undefined silently with no error message.
    Check isDefined() on the result if the path source is untrusted.

Source:
  ScriptingApi.cpp:7394  FileSystem::fromAbsolutePath()
    -> File::isAbsolutePath(path) check -> new ScriptFile(p, File(path))
