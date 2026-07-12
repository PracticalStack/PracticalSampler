Settings::setSampleFolder(ScriptObject sampleFolder) -> undefined

Thread safety: UNSAFE -- creates link files or sets frontend sample location, involving file I/O
Sets the sample folder location. Accepts a File object (not a string path). In
backend builds, creates a link file pointing to the new location. In frontend
builds, sets the sample location directly.

Anti-patterns:
  - Do NOT pass a string path -- requires a File object from FileSystem.
    String paths are silently ignored with no error.
  - [BUG] Silently does nothing if the argument is not a File object or if the
    path is not a directory. No error is reported.

Pair with:
  FileSystem.browseForDirectory -- get a File object from user selection
  FileSystem.getFolder -- get standard folder locations

Source:
  ScriptingApi.cpp  Settings::setSampleFolder()
    -> Backend: createLinkFile(FileHandlerBase::Samples, newLocation)
    -> Frontend: FrontendHandler::setSampleLocation(newLocation)
