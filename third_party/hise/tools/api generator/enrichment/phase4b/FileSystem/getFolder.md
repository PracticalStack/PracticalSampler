FileSystem::getFolder(Integer locationType) -> ScriptObject

Thread safety: UNSAFE -- allocates ScriptFile, resolves platform-specific paths, may create directories (AppData)
Returns a File object for the specified SpecialLocations constant. Primary way to obtain a
File handle for well-known system directories and HISE-managed folders. Resolution differs
between backend (IDE) and frontend (compiled plugin) builds for HISE-managed locations
(Samples, Expansions, AppData, UserPresets, AudioFiles). OS-mapped locations resolve the
same in all builds. Returns undefined if the resolved path does not exist as a directory.

Dispatch/mechanics:
  getFileStatic(locationType, mc) resolves via switch:
    HISE-managed: FileHandlerBase/FrontendHandler paths (backend/frontend differ)
    OS-mapped: File::getSpecialLocation() (Downloads is UserHome + "Downloads")
  -> isDirectory() check -> new ScriptFile or undefined

Pair with:
  findFiles -- scan files within the returned folder (getFolder is the prerequisite)
  fromAbsolutePath -- alternative for arbitrary paths
  fromReferenceString -- alternative for pool references

Anti-patterns:
  - Do NOT assume the result is always valid -- HISE-managed locations (Samples, AudioFiles)
    can return undefined if the folder does not exist. Check isDefined() before chaining.
  - Do NOT pass integers outside 0-11 -- no range check on the enum cast, undefined C++
    behavior in the switch statement.

Source:
  ScriptingApi.cpp:7394  FileSystem::getFolder()
    -> getFileStatic(locationType, mc) switch on SpecialLocations enum
    -> isDirectory() check -> new ScriptFile(p, file)
