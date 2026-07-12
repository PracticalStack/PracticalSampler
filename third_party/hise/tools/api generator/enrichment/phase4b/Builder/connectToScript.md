Builder::connectToScript(Integer buildIndex, String relativePath) -> undefined

Thread safety: UNSAFE -- involves file I/O via JavascriptProcessor::setConnectedFile.
Links the module at buildIndex to an external script file. Only works if the
target module is a JavascriptProcessor (ScriptProcessor, etc.). The relativePath
is relative to the project's Scripts folder.

Required setup:
  const var b = Synth.createBuilder();
  var midiIdx = b.create(b.MidiProcessors.ScriptProcessor, "MyMidi", synthIdx, b.ChainIndexes.Midi);

Pair with:
  create -- create the script processor first
  get -- retrieve a typed reference after connecting

Anti-patterns:
  - Silently does nothing if the module at buildIndex is not a JavascriptProcessor.
    No error message is reported. Always ensure the target is a script processor.

Source:
  ScriptingApiObjects.cpp  ScriptBuilder::connectToScript()
    -> dynamic_cast<JavascriptProcessor*>(p) -> setConnectedFile(relativePath, true)
