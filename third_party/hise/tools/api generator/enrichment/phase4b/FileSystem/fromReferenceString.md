FileSystem::fromReferenceString(String referenceStringOrFullPath, Integer locationType) -> ScriptObject

Thread safety: UNSAFE -- allocates ScriptFile, constructs PoolReference with string operations, accesses MainController
Resolves a HISE resource reference string (e.g. "{PROJECT_FOLDER}impulse.wav") or an absolute
path into a File object. Only three location types are valid: FileSystem.AudioFiles,
FileSystem.Samples, FileSystem.UserPresets. Others trigger a script error. Returns undefined
for embedded references (compiled into plugin binary) since no physical file exists.

Dispatch/mechanics:
  getSubdirectory(locationType) -> PoolReference(mc, refString, subDir)
  -> if ref.isAbsoluteFile(): new ScriptFile from path
  -> if ref.isValid() && !ref.isEmbeddedReference(): ref.getFile() -> new ScriptFile
  -> otherwise: return undefined

Pair with:
  fromAbsolutePath -- for plain absolute paths without pool resolution
  getFolder -- for SpecialLocations constants

Anti-patterns:
  - Do NOT use location types other than AudioFiles, Samples, or UserPresets -- triggers
    a script error with "X is not a valid locationType".
  - Do NOT rely on this in exported plugins for project-relative references -- they become
    embedded resources at export time and this method returns undefined. Code that works
    in the IDE may break in compiled plugins.

Source:
  ScriptingApi.cpp:7394  FileSystem::fromReferenceString()
    -> getSubdirectory() maps to FileHandlerBase::SubDirectories
    -> PoolReference(mc, refString, subDir) -> isAbsoluteFile/isEmbeddedReference checks
