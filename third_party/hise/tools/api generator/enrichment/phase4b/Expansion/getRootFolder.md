Expansion::getRootFolder() -> ScriptObject

Thread safety: UNSAFE -- heap allocation of ScriptFile object
Returns a File object pointing to the root folder of this expansion on disk.
Throws a script error ("Expansion was deleted") if the expansion reference has been invalidated.

Required setup:
  const var e = Engine.createExpansionHandler().getExpansionList()[0];

Pair with:
  getSampleFolder -- get the Samples subdirectory (may be redirected via link file)

Source:
  ScriptExpansion.cpp  ScriptExpansionReference::getRootFolder()
    -> checks objectExists() -> new ScriptFile(processor, exp->getRootFolder())
