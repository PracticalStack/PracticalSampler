Expansion::getWildcardReference(var relativePath) -> String

Thread safety: WARNING -- string construction and concatenation involve atomic ref-count operations
Constructs a wildcard reference string by prepending this expansion's wildcard prefix.
Result format: {EXP::ExpansionName}relativePath. Returns empty string if expansion is invalidated.

Required setup:
  const var e = Engine.createExpansionHandler().getExpansionList()[0];

Dispatch/mechanics:
  exp->getWildcard() + relativePath.toString()
    -> getWildcard() reads Name property: "{EXP::" + name + "}"

Pair with:
  getSampleMapList / getAudioFileList / getImageList / getMidiFileList -- get relative paths to wrap
  getDataFileList -- get data file paths to wrap

Source:
  ExpansionHandler.cpp:908  Expansion::getWildcard()
    -> "{EXP::" + getProperty(ExpansionIds::Name) + "}" + relativePath
