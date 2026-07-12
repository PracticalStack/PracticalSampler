Expansion::getSampleFolder() -> ScriptObject

Thread safety: UNSAFE -- heap allocation of ScriptFile object
Returns a File object pointing to the Samples subdirectory of this expansion.
If a link file redirects the Samples folder to a different location, the resolved target is returned.

Required setup:
  const var e = Engine.createExpansionHandler().getExpansionList()[0];

Pair with:
  setSampleFolder -- redirect the Samples folder to a new location
  getRootFolder -- get the expansion root directory

Anti-patterns:
  - [BUG] Does not check whether the expansion reference is still valid before access.
    If the expansion has been unloaded, this may crash instead of throwing a script error.

Source:
  ScriptExpansion.cpp  ScriptExpansionReference::getSampleFolder()
    -> exp->getSubDirectory(FileHandlerBase::Samples)
    -> new ScriptFile(processor, sampleFolder)
    -> does NOT call objectExists() (unlike getRootFolder)
