FileSystem::findFileSystemRoots() -> Array

Thread safety: UNSAFE -- allocates Array and ScriptFile objects on the heap
Returns an Array of File objects representing all root drives on the current computer.
On Windows: one entry per mounted drive (C:\, D:\, etc.). On macOS: typically a single
entry (/). Delegates to JUCE's File::findFileSystemRoots().

Pair with:
  findFiles -- scan files within a discovered root drive
  getFolder -- for well-known locations (preferred over root scanning)

Source:
  ScriptingApi.cpp:7394  FileSystem::findFileSystemRoots()
    -> File::findFileSystemRoots(roots) -> wraps each in ScriptFile
