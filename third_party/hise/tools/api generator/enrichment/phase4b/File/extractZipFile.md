File::extractZipFile(var targetDirectory, Integer overwriteFiles, Function callback) -> undefined

Thread safety: UNSAFE -- dispatches to Sample Loading Thread via killVoicesAndCall. Kills voices and blocks audio during extraction.
Extracts this ZIP archive to the target directory. Runs asynchronously on the
Sample Loading Thread. Callback fires with a status object at each extraction stage.
Callback signature: f(Object data)

Required setup:
  const var zipFile = FileSystem.getFolder(FileSystem.Downloads).getChildFile("samples.zip");
  const var targetDir = FileSystem.getFolder(FileSystem.Samples);

Dispatch/mechanics:
  killVoicesAndCall(SampleLoadingThread) -> creates ZipFile from f
  For <500 entries: callback per file. For >=500 entries: callback only for
  entries >200MB, with a 200ms PartUpdater timer for sub-entry progress.
  Updates SampleManager preload progress during extraction.
  Cancellation: checks thread exit, weak reference, and data.Cancel property.

Callback data properties:
  Status (int): 0=starting, 1=extracting, 2=complete
  Progress (double): 0.0 to 1.0
  TotalBytesWritten (int): running total of extracted bytes
  Cancel (int): set to true in callback to abort
  Target (string): target directory path
  CurrentFile (string): filename being extracted
  Error (string): error message on failure, empty on success

Pair with:
  getNumZippedItems -- to check archive size before extraction

Anti-patterns:
  - Audio output is silenced for the entire extraction duration (voices killed).
  - Setting Cancel=true stops extraction but already-extracted files remain on disk.
  - If the File object is garbage collected during async extraction, the operation
    aborts silently (weak reference check).

Source:
  ScriptingApiObjects.cpp  ScriptFile::extractZipFile()
    -> KillStateHandler::killVoicesAndCall(TargetThread::SampleLoadingThread)
    -> juce::ZipFile extraction loop with PartUpdater for large entries
    -> WeakCallbackHolder for safe callback invocation
