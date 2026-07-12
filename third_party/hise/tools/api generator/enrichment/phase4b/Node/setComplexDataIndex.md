Node::setComplexDataIndex(String dataType, Integer dataSlot, Integer indexValue) -> Integer

Thread safety: UNSAFE -- sets ValueTree property via undo manager.
Changes which external data slot a node references for a specific data type. Returns
true if set successfully, false if the node has no ComplexData tree, the type is
unrecognized, or the slot is out of range.

Valid dataType values: "Table", "SliderPack", "AudioFile", "FilterCoefficients", "DisplayBuffer"
Dispatch/mechanics:
  getValueTree()["ComplexData"] -> child(dataType + "s") -> child(dataSlot)
  -> setProperty(PropertyIds::Index, indexValue)
Source:
  NodeBase.cpp  NodeBase::setComplexDataIndex()
    -> ComplexData child -> appends "s" to dataType -> child at dataSlot
    -> sets PropertyIds::Index to indexValue
