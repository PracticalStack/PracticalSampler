FileSystem (namespace)

Namespace for accessing special folder locations, file browsing dialogs, filesystem
queries, and RSA encryption. Primary factory for File objects -- resolves special
folder constants, absolute paths, and HISE reference strings into file handles.
Abstracts backend (IDE) vs frontend (compiled plugin) directory differences.

Constants:
  SpecialLocations:
    AudioFiles = 0       Non-streaming audio files directory (impulse responses, loops)
    Expansions = 1       Expansion pack root folder
    Samples = 2          Sample files used by the streaming engine
    UserPresets = 3      User preset storage directory
    AppData = 4          Application data directory (Company/Product)
    UserHome = 5         User home directory
    Documents = 6        User documents directory
    Desktop = 7          User desktop directory
    Downloads = 8        User downloads directory
    Applications = 9     Global applications directory
    Temp = 10            System temp directory
    Music = 11           User music directory

Complexity tiers:
  1. Basic file access: getFolder + File.getChildFile(). Settings, favorites, or cache files in AppData.
  2. Content scanning: + findFiles with wildcard filtering. Sample browsers, IR selectors, preset explorers.
  3. File dialogs: + browse, browseForDirectory. User-initiated file import/export with async callback pattern.
  4. Reference resolution: + fromReferenceString. Converting {PROJECT_FOLDER} pool references to disk files.
  5. Encryption utilities: + getSystemId, encryptWithRSA, decryptWithRSA. Secure data exchange and machine identification.

Practical defaults:
  - Use FileSystem.AppData for any persistent data that is not a preset (settings, caches,
    favorites, MIDI mappings). Stable across plugin versions and platform-appropriate.
  - Use FileSystem.Samples as the start folder for browse dialogs when selecting audio content.
  - Cache findFiles() results to JSON in AppData for large content libraries. Re-scan only
    on explicit user action or content folder change.
  - Pass a File object (not a SpecialLocations constant) for the browse start folder when
    remembering the last-used directory. Store the path in settings, reconstruct with
    fromAbsolutePath().
  - Always check isDefined() on getFolder() results before chaining methods. HISE-managed
    locations (Samples, AudioFiles) can return undefined if the folder does not exist.

Common mistakes:
  - Passing a SpecialLocations constant directly to findFiles() -- silently returns empty
    array. Use getFolder() first to get a File object.
  - Using fromReferenceString() with a location type other than AudioFiles, Samples, or
    UserPresets -- triggers a script error.
  - Assuming browse() returns a value -- it is async. The File is delivered to the callback.
    Callback is not invoked if the user cancels.
  - Storing absolute paths in presets -- breaks on different machines or after sample folder
    relocation. Use File.toString(1) for relative paths.
  - Calling findFiles() on every UI refresh -- directory scanning is expensive, especially
    on network drives. Cache results.
  - Assuming getFolder(FileSystem.Samples) always returns a valid folder -- can be undefined
    with FullInstrumentExpansion active and no expansion loaded.

Example:
  // Get a reference to the user's desktop folder
  var desktop = FileSystem.getFolder(FileSystem.Desktop);

  // List all WAV files recursively
  var wavFiles = FileSystem.findFiles(desktop, "*.wav", true);

  // Browse for a file asynchronously
  FileSystem.browse(FileSystem.Desktop, false, "*.wav", function(f)
  {
      Console.print(f.toString(f.FullPath));
  });

Methods (15):
  browse                      browseForDirectory
  browseForMultipleDirectories browseForMultipleFiles
  decryptWithRSA              descriptionOfSizeInBytes
  encryptWithRSA              findFiles
  findFileSystemRoots         fromAbsolutePath
  fromReferenceString         getBytesFreeOnVolume
  getFolder                   getSystemId
  loadExampleAssets
