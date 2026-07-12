FileSystem::findFiles(ScriptObject directory, String wildcard, Integer recursive) -> Array

Thread safety: UNSAFE -- filesystem directory traversal, heap allocations (Array, ScriptFile construction), uses TimeoutExtender
Returns an Array of File objects for all matching children within the given directory.
The directory parameter must be a File object (not a SpecialLocations constant) -- use
getFolder() first. Finds both files and directories, ignores hidden files, filters out
.DS_Store. Returns empty array if argument is not a valid File directory.

Required setup:
  var folder = FileSystem.getFolder(FileSystem.Samples);

Dispatch/mechanics:
  dynamic_cast<ScriptFile*>(directory) -> root.isDirectory() check
  -> TimeoutExtender (prevents script timeout on large scans)
  -> root.f.findChildFiles(findFilesAndDirectories | ignoreHiddenFiles, recursive, wildcard)
  -> filters .DS_Store -> wraps each result in ScriptFile

Pair with:
  getFolder -- must convert SpecialLocations constant to File first
  findFileSystemRoots -- enumerate root drives before scanning

Anti-patterns:
  - Do NOT pass a SpecialLocations constant directly -- dynamic_cast<ScriptFile*> fails for
    integer values, silently returning an empty array. Always call getFolder() first.

Source:
  ScriptingApi.cpp:7394  FileSystem::findFiles()
    -> dynamic_cast<ScriptFile*> check -> TimeoutExtender
    -> File::findChildFiles() -> filter .DS_Store -> Array<ScriptFile>
