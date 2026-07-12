File::toReferenceString(String folderType) -> String

Thread safety: SAFE -- no I/O, constructs a pool reference string from in-memory path data.
Converts this file's absolute path to a HISE pool reference string relative to
the specified project subdirectory. Returns format: {PROJECT_FOLDER}filename.ext

Valid folderType values: AudioFiles, Images, SampleMaps, MidiFiles, UserPresets,
Samples, Scripts, Binaries, Presets, XmlPresetBackups, AdditionalSourceCode,
Documentation

Dispatch/mechanics:
  Appends "/" to folderType if missing -> matches against FileHandlerBase::getIdentifier()
  -> creates PoolReference(mainController, fullPath, subDir)
  -> returns ref.getReferenceString()

Pair with:
  FileSystem.fromReferenceString -- reverse conversion (reference to File)

Anti-patterns:
  - [BUG] "DspNetworks" cannot be used -- the internal identifier lacks a trailing
    slash, but this method auto-appends one, causing a mismatch. Results in
    "Illegal folder type" script error.
  - The file does not need to reside within the folder for the reference to be
    constructed, but the resulting reference will be meaningless if it does not.

Source:
  ScriptingApiObjects.cpp  ScriptFile::toReferenceString()
    -> FileHandlerBase::getIdentifier() matching loop
    -> PoolReference(mc, fullPath, dirToUse).getReferenceString()
