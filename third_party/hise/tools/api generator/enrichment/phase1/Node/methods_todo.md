# Node -- Method Workbench

## Progress
- [x] connectTo
- [x] connectToBypass
- [x] get
- [x] getChildNodes
- [x] getIndexInParent
- [x] getNodeHolder
- [x] getNumParameters
- [x] getOrCreateParameter
- [x] isActive
- [x] isBypassed
- [x] reset
- [x] set
- [x] setBypassed
- [x] setComplexDataIndex
- [x] setParent

## Forced Parameter Types

No methods use `ADD_TYPED_API_METHOD_N`. All 13 registered methods use plain `ADD_API_METHOD_N` -- parameter types must be inferred from the C++ signatures.

Note: `getIndexInParent`, `isActive`, and `getNodeHolder` appear in the base JSON but have no `ADD_API_METHOD` registration or Wrapper entry in the C++ source. They may not be callable from HISEScript.
