# TransportHandler -- Method Workbench

## Progress
- [x] setOnTempoChange
- [x] setOnTransportChange
- [x] setOnBeatChange
- [x] setOnGridChange
- [x] setOnSignatureChange
- [x] setOnBypass
- [x] setSyncMode
- [x] startInternalClock
- [x] stopInternalClock
- [x] stopInternalClockOnExternalStop
- [x] setLinkBpmToSyncMode
- [x] setEnableGrid
- [x] setLocalGridMultiplier
- [x] setLocalGridBypassed
- [x] sendGridSyncOnNextCallback
- [x] isPlaying
- [x] isNonRealtime
- [x] getGridLengthInSamples
- [x] getGridPosition

## Forced Parameter Types
| Method | Param 1 | Param 2 |
|--------|---------|---------|
| setOnTempoChange | Number | Function |
| setOnBeatChange | Number | Function |
| setOnGridChange | Number | Function |
| setOnSignatureChange | Number | Function |
| setOnTransportChange | Number | Function |
| setOnBypass | Function | -- |

(Methods not listed here use plain ADD_API_METHOD_N -- types must be inferred.)
