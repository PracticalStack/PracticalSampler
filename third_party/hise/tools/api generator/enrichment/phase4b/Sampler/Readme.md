Sampler (object)
Obtain via: Sampler (auto-registered in sampler modules) or ChildSynth.asSampler()

Script handle for ModulatorSampler: sample map loading/saving, round-robin group
management, multi-mic position control, sample selection and property editing,
timestretching, and release-start configuration.

Constants:
  SampleProperty:
    FileName = 1              Audio file path
    Root = 2                  Root note
    HiKey = 3                 Highest mapped key
    LoKey = 4                 Lowest mapped key
    LoVel = 5                 Lowest mapped velocity
    HiVel = 6                 Highest mapped velocity
    RRGroup = 7               Round-robin group index
    Volume = 8                Gain in decibels
    Pan = 9                   Stereo panning
    Normalized = 10           Enable sample normalization
    Pitch = 11                Pitch factor in cents (+/- 100)
    SampleStart = 12          Start sample offset
    SampleEnd = 13            End sample offset
    SampleStartMod = 14       Sample start modulation range
    LoopStart = 15            Loop start in samples
    LoopEnd = 16              Loop end in samples
    LoopXFade = 17            Loop crossfade length
    LoopEnabled = 18          Enable sample looping
    ReleaseStart = 19         Release trigger offset in samples
    LowerVelocityXFade = 20   Lower velocity crossfade length
    UpperVelocityXFade = 21   Upper velocity crossfade length
    SampleState = 22          Sample state (Normal/Disabled/Purged)
    Reversed = 23             Play sample in reverse
    NumQuarters = 24          Length in quarter notes (tempo-synced stretching)

Complexity tiers:
  1. Basic sampler: loadSampleMap, getSampleMapList, getCurrentSampleMapId.
     Single-sampler instrument with preset-driven map selection.
  2. Multi-mic sampler: + purgeMicPosition, getNumMicPositions, getMicPositionName,
     isMicPositionPurged. Requires mic suffix naming and purge/gain coordination.
  3. Manual round-robin: + enableRoundRobin, setActiveGroup, refreshRRMap,
     getRRGroupsForMessage. Script-driven group selection based on musical context.
  4. User sample import: + parseSampleFile, loadSampleMapFromJSON,
     getSampleMapAsBase64, loadSampleMapFromBase64, clearSampleMap, createSelection.
     Full user-importable content workflow with preset persistence.
  5. Advanced sample manipulation: + createSelectionWithFilter, Sample.set,
     refreshInterface. IDE tooling scripts for programmatic property modification.

Practical defaults:
  - Use Synth.getSampler("Name") and cache as const var when the script is not
    inside the sampler module.
  - Call enableRoundRobin(false) in onInit before any manual group selection.
  - Use timer-deferred loading (50ms poll) when loadSampleMap is triggered by UI
    controls to avoid audio glitches during preloading.
  - Prefer createSelection(".*") or createSelectionFromIndexes(-1) over the legacy
    selectSounds() workflow.
  - Call refreshInterface() after modifying sample properties via Sample.set().

Common mistakes:
  - Calling setActiveGroup/setMultiGroupIndex/getRRGroupsForMessage without
    enableRoundRobin(false) first -- throws script error.
  - Calling setActiveGroupForEventId with eventId != -1 outside onNoteOn -- throws
    "only available in onNoteOnCallback" error.
  - Calling getRRGroupsForMessage without refreshRRMap() first -- stale/empty data.
  - Calling loadSampleMap directly from a ComboBox callback -- causes audio glitches;
    use a timer to defer (50ms poll).
  - Using repeated Content.getComponent lookups instead of caching sampler references
    as const var in onInit.
  - Parameter order mismatch: setSoundProperty(soundIndex, propertyIndex, value) vs
    getSoundProperty(propertyIndex, soundIndex) -- reversed order causes silent bugs.

Example:
  // The Sampler object is auto-available when the script is inside a Sampler module
  const var sampler = Sampler;

  // Load a sample map
  sampler.loadSampleMap("MyInstrument");

  // Create a selection of all samples and modify a property
  const var allSamples = sampler.createSelectionFromIndexes(-1);

Methods (57):
  clearSampleMap                createListFromGUISelection
  createListFromScriptSelection createSelection
  createSelectionFromIndexes    createSelectionWithFilter
  enableRoundRobin              getActiveRRGroup
  getActiveRRGroupForEventId    getAttribute
  getAttributeId                getAttributeIndex
  getAudioWaveformContentAsBase64  getComplexGroupManager
  getCurrentSampleMapId         getMicPositionName
  getNumActiveGroups            getNumAttributes
  getNumMicPositions            getNumSelectedSounds
  getRRGroupsForMessage         getReleaseStartOptions
  getSampleMapAsBase64          getSampleMapList
  getSoundProperty              getTimestretchOptions
  importSamples                 isMicPositionPurged
  isNoteNumberMapped            loadSampleForAnalysis
  loadSampleMap                 loadSampleMapFromBase64
  loadSampleMapFromJSON         loadSfzFile
  parseSampleFile               purgeMicPosition
  purgeSampleSelection          refreshInterface
  refreshRRMap                  saveCurrentSampleMap
  selectSounds                  setActiveGroup
  setActiveGroupForEventId      setAllowReleaseStart
  setAttribute                  setGUISelection
  setMultiGroupIndex            setMultiGroupIndexForEventId
  setRRGroupVolume              setReleaseStartOptions
  setSortByRRGroup              setSoundProperty
  setSoundPropertyForAllSamples setSoundPropertyForSelection
  setTimestretchOptions         setTimestretchRatio
  setUseStaticMatrix
