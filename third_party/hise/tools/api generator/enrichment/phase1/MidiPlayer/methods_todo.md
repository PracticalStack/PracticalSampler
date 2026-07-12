# MidiPlayer -- Method Workbench

## Progress
- [x] asMidiProcessor
- [x] clearAllSequences
- [x] connectToMetronome
- [x] connectToPanel
- [x] convertEventListToNoteRectangles
- [x] create
- [x] flushMessageList
- [x] flushMessageListToSequence
- [x] getEventList
- [x] getEventListFromSequence
- [x] getLastPlayedNotePosition
- [x] getMidiFileList
- [x] getNoteRectangleList
- [x] getNumSequences
- [x] getNumTracks
- [x] getPlaybackPosition
- [x] getPlayState
- [x] getTicksPerQuarter
- [x] getTimeSignature
- [x] getTimeSignatureFromSequence
- [x] isEmpty
- [x] isSequenceEmpty
- [x] play
- [x] record
- [x] redo
- [x] reset
- [x] saveAsMidiFile
- [x] setAutomationHandlerConsumesControllerEvents
- [x] setFile
- [x] setGlobalPlaybackRatio
- [x] setPlaybackCallback
- [x] setPlaybackPosition
- [x] setRecordEventCallback
- [x] setRepaintOnPositionChange
- [x] setSequence
- [x] setSequenceCallback
- [x] setSyncToMasterClock
- [x] setTimeSignature
- [x] setTimeSignatureToSequence
- [x] setTrack
- [x] setUseGlobalUndoManager
- [x] setUseTimestampInTicks
- [x] stop
- [x] undo

## Forced Parameter Types
| Method | Param 1 | Param 2 |
|--------|---------|---------|
| setSequenceCallback | Function | -- |
| setPlaybackCallback | Function | Number |
| setRecordEventCallback | Function | -- |

(Methods not listed here use plain ADD_API_METHOD_N -- types must be inferred.)
