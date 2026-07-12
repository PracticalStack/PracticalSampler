# RoutingMatrix -- Method Workbench

## Progress
- [x] addConnection
- [x] addSendConnection
- [x] clear
- [x] getDestinationChannelForSource
- [x] getNumDestinationChannels
- [x] getNumSourceChannels
- [x] getSourceChannelsForDestination
- [x] getSourceGainValue
- [x] removeConnection
- [x] removeSendConnection
- [x] setForcePeakMeters
- [x] setNumChannels

## Forced Parameter Types

No methods use `ADD_TYPED_API_METHOD_N`. All methods use plain `ADD_API_METHOD_N` -- parameter types must be inferred from C++ signatures.

## Notes

- `setForcePeakMeters` exists in the base JSON but has NO C++ implementation anywhere in the HISE source. It is a phantom method -- likely planned but never implemented, or removed. The method should be flagged as non-functional during Step B analysis.
