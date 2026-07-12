Expansion::setSampleFolder(var newSampleFolder) -> bool

Thread safety: UNSAFE -- creates a link file on disk and refreshes subdirectory state
Redirects this expansion's Samples folder to a new location by creating a link file.
Returns true when a link file was successfully created. Returns false if the target
is the same as the current folder or if the argument is not a File object.

Required setup:
  const var e = Engine.createExpansionHandler().getExpansionList()[0];

Dispatch/mechanics:
  dynamic_cast<ScriptFile*>(newSampleFolder)
    -> validates newTarget.isDirectory()
    -> exp->createLinkFile(FileHandlerBase::Samples, newTarget)
    -> exp->checkSubDirectories()

Pair with:
  getSampleFolder -- read the current (possibly redirected) Samples folder

Anti-patterns:
  - Do NOT pass a string path -- requires a File object (from FileSystem.getFolder()
    or similar). Passing a string silently returns false because the internal
    dynamic_cast to ScriptFile fails.
  - [BUG] Does not check whether the expansion reference is still valid. If the
    expansion has been unloaded, this may crash.

Source:
  ScriptExpansion.cpp:1733  ScriptExpansionReference::setSampleFolder()
    -> dynamic_cast<ScriptFile*>(newSampleFolder)
    -> exp->createLinkFile(FileHandlerBase::Samples, newTarget)
    -> exp->checkSubDirectories()
