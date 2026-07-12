# DspNetwork -- Method Workbench

## Progress
- [x] clear
- [x] create
- [x] createAndAdd
- [x] createFromJSON
- [x] createTest
- [x] deleteIfUnused
- [x] get
- [x] prepareToPlay
- [x] processBlock
- [x] setForwardControlsToParameters
- [x] setParameterDataFromJSON
- [x] undo

## Forced Parameter Types
No methods use ADD_TYPED_API_METHOD_N. All 11 registered methods use plain ADD_API_METHOD_N -- parameter types must be inferred from C++ signatures.

Note: `deleteIfUnused` has a Doxygen comment but is NOT in the ADD_API_METHOD registration list. It may not be callable from script. Verify during Step B.
