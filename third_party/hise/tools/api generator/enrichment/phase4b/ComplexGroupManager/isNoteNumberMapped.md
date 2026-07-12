ComplexGroupManager::isNoteNumberMapped(Number layerIndex, Number noteNumber) -> Integer

Thread safety: SAFE
Returns 1 if any samples in the specified layer cover the given MIDI note number, 0
otherwise. Requires createNoteMap() to have been called first for the same layer;
throws a script error if the note map has not been built.

Required setup:
  const var cgm = Sampler.getComplexGroupManager();
  cgm.createNoteMap(0);
  // ... then:
  var mapped = cgm.isNoteNumberMapped(0, 60);

Pair with:
  createNoteMap -- must be called first (script error otherwise)

Anti-patterns:
  - Do NOT call without createNoteMap() first -- throws a script error
  - Do NOT assume the note map stays valid after sample map changes -- call
    createNoteMap() again after loading a new sample map

Source:
  ScriptingApiObjects.cpp  ScriptingComplexGroupManager::isNoteNumberMapped()
    -> looks up noteMaps by layer index -> checks VoiceBitMap at noteNumber
