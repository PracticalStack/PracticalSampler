Engine (namespace)

Central factory, utility, and system-query namespace for object creation,
unit conversion, and global engine state. The largest API class in HISE with
142 methods spanning object factories, audio/musical unit conversions, user
preset management, pool operations, system queries, and dialogs.

Complexity tiers:
  1. Basic utilities: getSampleRate, getOS, isPlugin, loadFontAs, setGlobalFont,
     loadAudioFilesIntoPool, getFrequencyForMidiNoteNumber, getDecibelsForGainFactor.
     Every plugin uses a subset of these for initialization and basic math.
  2. Factory creation: createTimerObject, createBroadcaster, createUserPresetHandler,
     createMidiList, createTransportHandler, createMacroHandler. Entry points for
     major subsystems -- most intermediate plugins use at least timers and preset handlers.
  3. Complex data and rendering: createFixObjectFactory, createUnorderedStack,
     createMessageHolder, createBackgroundTask, renderAudio, createModulationMatrix,
     createDspNetwork. Advanced architectures: granular synthesis, offline rendering,
     dynamic modulation routing, and scriptnode integration.

Practical defaults:
  - Use Engine.loadFontAs("{PROJECT_FOLDER}Fonts/MyFont.ttf", "MyFont") with an
    explicit font ID rather than relying on the font's internal name.
  - Call Engine.loadAudioFilesIntoPool() in onInit for any plugin that references
    audio files from scripts.
  - Use Engine.createTransportHandler() instead of Engine.getPlayHead() for host
    transport information. The playhead object's properties are not populated.
  - When using Engine.setKeyColour(), iterate the full 0-127 range and set every
    key -- including unmapped keys to a dimmed colour like 0x99444444.
  - Use Engine.createTimerObject() with startTimer(500) for CPU/peak meter polling.

Common mistakes:
  - Calling Engine.loadUserPreset() in onInit -- explicitly rejected with script
    error. Load presets from runtime callbacks (button handlers, timers).
  - Using Engine.getPlayHead().bpm -- the playhead property-population code is
    commented out; use getHostBpm() or createTransportHandler() instead.
  - Passing Engine.getMacroName(0) -- macro indices are 1-based (1-8), not 0-based.
  - Calling Engine.setFrontendMacros() with fewer names than HISE_NUM_MACROS --
    excess slots silently receive empty-string names.
  - Using Engine.addModuleStateToUserPreset("MyEQ") with just a string -- pass a
    JSON object with RemovedProperties to exclude noise like routing matrix state.
  - Calling Engine.setKeyColour() only for mapped notes -- skipping unmapped keys
    leaves stale colours from a previous state.

Example:
  // Engine is an implicit global -- no creation needed.
  // Unit conversion
  var freq = Engine.getFrequencyForMidiNoteNumber(69); // 440.0
  var db = Engine.getDecibelsForGainFactor(0.5);        // ~-6.0

  // System queries
  var sr = Engine.getSampleRate();
  var os = Engine.getOS(); // "WIN", "OSX", or "LINUX"

Methods (142):
  addModuleStateToUserPreset      allNotesOff
  clearMidiFilePool               clearSampleMapPool
  clearUndoHistory                compressJSON
  copyToClipboard                 createAndRegisterAudioFile
  createAndRegisterRingBuffer     createAndRegisterSliderPackData
  createAndRegisterTableData      createBackgroundTask
  createBXLicenser                createBeatportManager
  createBroadcaster               createDspNetwork
  createErrorHandler              createExpansionHandler
  createFFT                       createFixObjectFactory
  createGlobalScriptLookAndFeel   createLicenseUnlocker
  createMacroHandler              createMessageHolder
  createMidiAutomationHandler     createMidiList
  createModulationMatrix          createNKSManager
  createNeuralNetwork             createThreadSafeStorage
  createTimerObject               createTransportHandler
  createUnorderedStack            createUserPresetHandler
  decodeBase64ValueTree           doubleToString
  dumpAsJSON                      extendTimeOut
  getBufferSize                   getClipboardContent
  getComplexDataReference         getControlRateDownsamplingFactor
  getCpuUsage                     getCurrentUserPresetName
  getDecibelsForGainFactor        getDeviceResolution
  getDeviceType                   getDspNetworkReference
  getExpansionList                getExtraDefinitionsInBackend
  getFilterModeList               getFrequencyForMidiNoteNumber
  getGainFactorForDecibels        getGlobalPitchFactor
  getGlobalRoutingManager         getHostBpm
  getLatencySamples               getLorisManager
  getMacroName                    getMasterPeakLevel
  getMemoryUsage                  getMidiNoteFromName
  getMidiNoteName                 getMilliSecondsForQuarterBeats
  getMilliSecondsForQuarterBeatsWithTempo
  getMilliSecondsForSamples       getMilliSecondsForTempo
  getName                         getNumPluginChannels
  getNumVoices                    getOS
  getPitchRatioFromSemitones      getPlayHead
  getPreloadMessage               getPreloadProgress
  getProjectInfo                  getQuarterBeatsForMilliSeconds
  getQuarterBeatsForMilliSecondsWithTempo
  getQuarterBeatsForSamples       getQuarterBeatsForSamplesWithTempo
  getRegexMatches                 getSampleFilesFromDirectory
  getSampleRate                   getSamplesForMilliSeconds
  getSamplesForQuarterBeats       getSamplesForQuarterBeatsWithTempo
  getSemitonesFromPitchRatio      getStringWidth
  getSystemStats                  getSystemTime
  getTempoName                    getTextForValue
  getUptime                       getUserPresetList
  getValueForText                 getVersion
  getWavetableList                intToHexString
  isControllerUsedByAutomation    isHISE
  isMpeEnabled                    isPlugin
  isUserPresetReadOnly            loadAudioFileIntoBufferArray
  loadAudioFilesIntoPool          loadFontAs
  loadFromJSON                    loadImageIntoPool
  loadNextUserPreset              loadPreviousUserPreset
  loadUserPreset                  logSettingWarning
  matchesRegex                    openWebsite
  performUndoAction               playBuffer
  quit                            rebuildCachedPools
  redo                            reloadAllSamples
  renderAudio                     saveUserPreset
  setAllowDuplicateSamples        setCurrentExpansion
  setFrontendMacros               setGlobalFont
  setGlobalPitchFactor            setHostBpm
  setKeyColour                    setLatencySamples
  setLowestKeyToDisplay           setMaximumBlockSize
  setMinimumSampleRate            setPreloadMessage
  setUserPresetTagList            showErrorMessage
  showMessage                     showMessageBox
  showYesNoWindow                 sortWithFunction
  uncompressJSON                  undo
