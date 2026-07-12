Parameter::setRangeFromObject(var propertyObject) -> undefined

Thread safety: UNSAFE -- modifies ValueTree properties with undo support.
Updates the parameter's range from a JSON object. Missing properties receive
defaults: MinValue=0.0, MaxValue=1.0, SkewFactor=1.0, StepSize=0.0, Inverted=false.
Supports undo via the parent node's UndoManager.

Required setup:
  const var p = nd.getOrCreateParameter("Frequency");

Dispatch/mechanics:
  Reads JSON properties with defaults -> checkIfIdentity() on range
    -> RangeHelpers::storeDoubleRange(data, range, parent->getUndoManager())
    -> ValueTree property changes trigger rangeListener -> dynamicParameter->updateRange()

Pair with:
  getRangeObject -- read the current range before modifying
  setRangeProperty -- set individual properties (no undo)

Source:
  NodeBase.cpp  Parameter::setRangeFromObject()
    -> RangeHelpers::storeDoubleRange(data, range, undoManager)
