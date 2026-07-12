Expansion (object)
Obtain via: ExpansionHandler.getExpansion(expansionName) or ExpansionHandler.getExpansionList()

Handle to a single installed expansion pack. Provides read access to resource pools
(sample maps, audio files, images, MIDI files, data files, user presets), metadata
properties, and folder locations. Supports JSON data file I/O, sample folder
redirection, and user preset extraction from encoded expansions.

Complexity tiers:
  1. Resource listing: getProperties, getSampleMapList, getAudioFileList, getImageList,
     getMidiFileList, getUserPresetList. Read metadata and enumerate content per expansion.
  2. Resource referencing: + getWildcardReference. Build {EXP::Name}path pool references
     for loading expansion resources into UI components or audio processors.
  3. Data persistence: + loadDataFile, writeDataFile, getDataFileList. Store and retrieve
     per-expansion JSON configuration in the AdditionalSourceCode folder.
  4. Advanced management: + setSampleFolder, rebuildUserPresets, setAllowDuplicateSamples,
     unloadExpansion, getExpansionType, getRootFolder, getSampleFolder. Filesystem
     redirection, preset extraction, sample deduplication, and lifecycle.

Practical defaults:
  - Use getProperties().Name to display expansion names in combo boxes and browser UIs.
  - Use getWildcardReference() to build image paths for expansion-specific theming;
    fall back to {PROJECT_FOLDER} references when no expansion is active.
  - Combine getSampleMapList() from each expansion with Sampler.getSampleMapList()
    for embedded content to build a complete sample map browser.
  - Iterate all expansions in onInit to build resource lists; use
    ExpansionHandler.setExpansionCallback() to react to expansion changes at runtime.

Common mistakes:
  - Passing a string path to setSampleFolder() instead of a File object -- silently
    returns false. Use FileSystem.getFolder() to obtain a File object.
  - Calling rebuildUserPresets() on a FileBased expansion -- silently returns false.
    Only works on Intermediate/Encrypted expansions.
  - Using expansion resources without handling the "no expansion" case -- the expansion
    callback receives undefined when no expansion is active. Check isDefined(expansion)
    and fall back to {PROJECT_FOLDER} references.
  - Building resource lists only from expansions -- include Sampler.getSampleMapList()
    or Engine.loadAudioFilesIntoPool() for embedded root content. Expansions supplement
    the project's own resources.

Example:
  // Get an expansion from the handler
  const var eh = Engine.createExpansionHandler();
  const var list = eh.getExpansionList();

  if (list.length > 0)
  {
      const var e = list[0];
      var props = e.getProperties();
      Console.print("Expansion: " + props.Name + " v" + props.Version);

      // List available sample maps
      var maps = e.getSampleMapList();
      for (m in maps)
          Console.print("  SampleMap: " + m);
  }

Methods (17):
  getAudioFileList        getDataFileList         getExpansionType
  getImageList            getMidiFileList          getProperties
  getRootFolder           getSampleFolder          getSampleMapList
  getUserPresetList       getWildcardReference     loadDataFile
  rebuildUserPresets       setAllowDuplicateSamples setSampleFolder
  unloadExpansion         writeDataFile
