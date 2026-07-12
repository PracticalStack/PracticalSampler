File (object)
Obtain via: FileSystem.getFolder(location)

Filesystem handle for reading, writing, and manipulating files and directories.
Wraps juce::File with methods for text, JSON, encrypted, audio, MIDI, XML, and
Base64 I/O plus copy/move/rename/delete operations. The internal path is
immutable -- after rename/move/delete, the object still references the old path.

Constants:
  Format:
    FullPath = 0        Full absolute path
    NoExtension = 1     Filename without extension
    Extension = 2       File extension only (e.g., ".wav")
    Filename = 3        Filename with extension

Complexity tiers:
  1. Basic persistence: loadAsObject, writeObject, getChildFile, isFile,
     deleteFileOrDirectory, toString. Every plugin that persists custom data.
  2. File management: + createDirectory, getParentDirectory,
     getNonExistentSibling, getRedirectedFolder, toReferenceString,
     loadAudioMetadata. Custom file browsers or sample management.
  3. Installation workflows: + extractZipFile, getSize, getBytesFreeOnVolume,
     writeString (for link files), hasWriteAccess. Sample installer plugins.
  4. Audio/MIDI I/O: + loadAsAudioFile, writeAudioFile, loadAsMidiFile,
     writeMidiFile. Plugins that export or preview audio/MIDI content.

Practical defaults:
  - Use FileSystem.getFolder(FileSystem.AppData).getChildFile("settings.json")
    for persistent settings that survive preset changes.
  - Always check isDefined() on loadAsObject/loadFromXmlFile results before
    accessing properties -- file may not exist on first run.
  - Use getNonExistentSibling() before writeAudioFile to avoid silently
    overwriting existing files.
  - Check getBytesFreeOnVolume() on target drive before starting ZIP extraction.
  - Use toString(0) (FullPath) for lookup keys, toString(1) (NoExtension) for
    display names without extension.

Common mistakes:
  - Using the File object after rename/move/delete -- the internal path is
    immutable. Obtain a fresh File handle for the new location.
  - Calling loadAsObject on first run without a fallback -- file does not exist
    yet, returns undefined. Always provide a default: if (!isDefined(data)) data = {};
  - Confusing writeEncryptedObject (BlowFish symmetric, max 72-byte key) with
    FileSystem.encryptWithRSA (public-key). They are different encryption schemes.
  - Using toString(0) as a portable key across machines -- full paths are
    machine-specific. Use getRelativePathFrom or toReferenceString instead.

Example:
  // Get a file handle and perform basic I/O
  var f = FileSystem.getFolder(FileSystem.Documents).getChildFile("data.json");
  f.writeObject({ "name": "test", "value": 42 });
  var data = f.loadAsObject();

Methods (42):
  copy                    copyDirectory           createDirectory
  deleteFileOrDirectory   extractZipFile          getBytesFreeOnVolume
  getChildFile            getHash                 getNonExistentSibling
  getNumZippedItems       getParentDirectory      getRedirectedFolder
  getRelativePathFrom     getSize                 hasWriteAccess
  isChildOf               isDirectory             isFile
  isSameFileAs            loadAsAudioFile         loadAsBase64String
  loadAsMidiFile          loadAsObject            loadAsString
  loadAudioMetadata       loadEncryptedObject     loadFromXmlFile
  loadMidiMetadata        move                    rename
  setExecutePermission    setReadOnly             show
  startAsProcess          toReferenceString       toString
  writeAsXmlFile          writeAudioFile          writeEncryptedObject
  writeMidiFile           writeObject             writeString
