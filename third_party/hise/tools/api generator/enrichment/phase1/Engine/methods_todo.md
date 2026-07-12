# Engine -- Method Workbench

## Progress
- [x] addModuleStateToUserPreset
- [x] allNotesOff
- [x] clearMidiFilePool
- [x] clearSampleMapPool
- [x] clearUndoHistory
- [x] compressJSON
- [x] copyToClipboard
- [x] createAndRegisterAudioFile
- [x] createAndRegisterRingBuffer
- [x] createAndRegisterSliderPackData
- [x] createAndRegisterTableData
- [x] createBackgroundTask
- [x] createBeatportManager
- [x] createBroadcaster
- [x] createBXLicenser
- [x] createDspNetwork
- [x] createErrorHandler
- [x] createExpansionHandler
- [x] createFFT
- [x] createFixObjectFactory
- [x] createGlobalScriptLookAndFeel
- [x] createLicenseUnlocker
- [x] createMacroHandler
- [x] createMessageHolder
- [x] createMidiAutomationHandler
- [x] createMidiList
- [x] createModulationMatrix
- [x] createNeuralNetwork
- [x] createNKSManager
- [x] createThreadSafeStorage
- [x] createTimerObject
- [x] createTransportHandler
- [x] createUnorderedStack
- [x] createUserPresetHandler
- [x] decodeBase64ValueTree
- [x] doubleToString
- [x] dumpAsJSON
- [x] extendTimeOut
- [x] getBufferSize
- [x] getClipboardContent
- [x] getComplexDataReference
- [x] getControlRateDownsamplingFactor
- [x] getCpuUsage
- [x] getCurrentUserPresetName
- [x] getDecibelsForGainFactor
- [x] getDeviceResolution
- [x] getDeviceType
- [x] getDspNetworkReference
- [x] getExpansionList
- [x] getExtraDefinitionsInBackend
- [x] getFilterModeList
- [x] getFrequencyForMidiNoteNumber
- [x] getGainFactorForDecibels
- [x] getGlobalPitchFactor
- [x] getGlobalRoutingManager
- [x] getHostBpm
- [x] getLatencySamples
- [x] getLorisManager
- [x] getMacroName
- [x] getMasterPeakLevel
- [x] getMemoryUsage
- [x] getMidiNoteFromName
- [x] getMidiNoteName
- [x] getMilliSecondsForQuarterBeats
- [x] getMilliSecondsForQuarterBeatsWithTempo
- [x] getMilliSecondsForSamples
- [x] getMilliSecondsForTempo
- [x] getName
- [x] getNumPluginChannels
- [x] getNumVoices
- [x] getOS
- [x] getPitchRatioFromSemitones
- [x] getPlayHead
- [x] getPreloadMessage
- [x] getPreloadProgress
- [x] getProjectInfo
- [x] getQuarterBeatsForMilliSeconds
- [x] getQuarterBeatsForMilliSecondsWithTempo
- [x] getQuarterBeatsForSamples
- [x] getQuarterBeatsForSamplesWithTempo
- [x] getRegexMatches
- [x] getSampleFilesFromDirectory
- [x] getSampleRate
- [x] getSamplesForMilliSeconds
- [x] getSamplesForQuarterBeats
- [x] getSamplesForQuarterBeatsWithTempo
- [x] getSemitonesFromPitchRatio
- [x] getSettingsWindowObject
- [x] getStringWidth
- [x] getSystemStats
- [x] getSystemTime
- [x] getTempoName
- [x] getTextForValue
- [x] getUptime
- [x] getUserPresetList
- [x] getValueForText
- [x] getVersion
- [x] getWavetableList
- [x] getZoomLevel
- [x] intToHexString
- [x] isControllerUsedByAutomation
- [x] isHISE
- [x] isMpeEnabled
- [x] isPlugin
- [x] isUserPresetReadOnly
- [x] loadAudioFileIntoBufferArray
- [x] loadAudioFilesIntoPool
- [x] loadFont
- [x] loadFontAs
- [x] loadFromJSON
- [x] loadImageIntoPool
- [x] loadNextUserPreset
- [x] loadPreviousUserPreset
- [x] loadUserPreset
- [x] logSettingWarning
- [x] matchesRegex
- [x] openWebsite
- [x] performUndoAction
- [x] playBuffer
- [x] quit
- [x] rebuildCachedPools
- [x] redo
- [x] reloadAllSamples
- [x] renderAudio
- [x] saveUserPreset
- [x] setAllowDuplicateSamples
- [x] setCurrentExpansion
- [x] setDiskMode
- [x] setFrontendMacros
- [x] setGlobalFont
- [x] setGlobalPitchFactor
- [x] setHostBpm
- [x] setKeyColour
- [x] setLatencySamples
- [x] setLowestKeyToDisplay
- [x] setMaximumBlockSize
- [x] setMinimumSampleRate
- [x] setPreloadMessage
- [x] setUserPresetTagList
- [x] setZoomLevel
- [x] showErrorMessage
- [x] showMessage
- [x] showMessageBox
- [x] showYesNoWindow
- [x] sortWithFunction
- [x] uncompressJSON
- [x] undo

## Forced Parameter Types
| Method | Param 1 | Param 2 | Param 3 | Param 4 | Param 5 |
|--------|---------|---------|---------|---------|---------|
| getDecibelsForGainFactor | Number | -- | -- | -- | -- |
| getFrequencyForMidiNoteNumber | Number | -- | -- | -- | -- |
| getGainFactorForDecibels | Number | -- | -- | -- | -- |
| getMilliSecondsForQuarterBeats | Number | -- | -- | -- | -- |
| getMilliSecondsForSamples | Number | -- | -- | -- | -- |
| getMilliSecondsForTempo | Number | -- | -- | -- | -- |
| getPitchRatioFromSemitones | Number | -- | -- | -- | -- |
| getQuarterBeatsForMilliSeconds | Number | -- | -- | -- | -- |
| getQuarterBeatsForSamples | Number | -- | -- | -- | -- |
| getSamplesForMilliSeconds | Number | -- | -- | -- | -- |
| getSamplesForQuarterBeats | Number | -- | -- | -- | -- |
| getSemitonesFromPitchRatio | Number | -- | -- | -- | -- |
| performUndoAction | JSON | Function | -- | -- | -- |
| setHostBpm | Number | -- | -- | -- | -- |
| setMaximumBlockSize | Number | -- | -- | -- | -- |
| setMinimumSampleRate | Number | -- | -- | -- | -- |
