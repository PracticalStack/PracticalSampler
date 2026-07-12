ComplexGroupManager::createNoteMap(var layerIdOrIndex) -> undefined

Thread safety: UNSAFE -- iterates all samples in the sample map and allocates a VoiceBitMap for lookup storage
Builds a note-to-group mapping for the specified layer by scanning all samples in the
current sample map. Stores a 128-entry bitmap used by isNoteNumberMapped() for fast
lookups. Must be called before isNoteNumberMapped() and again after sample map changes.

Required setup:
  const var cgm = Sampler.getComplexGroupManager();

Dispatch/mechanics:
  Iterates all samples in sample map -> checks each sample's layer value
    -> if non-zero and non-IgnoreFlag, marks note range in VoiceBitMap<128, uint32>
    -> stores map in noteMaps keyed by layer index

Pair with:
  isNoteNumberMapped -- requires createNoteMap() to be called first

Source:
  ScriptingApiObjects.cpp  ScriptingComplexGroupManager::createNoteMap()
    -> iterates sampler sounds -> builds VoiceBitMap<128, uint32> -> stores in noteMaps
