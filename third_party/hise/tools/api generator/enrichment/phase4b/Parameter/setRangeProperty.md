Parameter::setRangeProperty(String id, var newValue) -> undefined

Thread safety: UNSAFE -- modifies ValueTree property with notification chain.
Sets a single range property by ID string. Valid IDs: MinValue, MaxValue, StepSize,
SkewFactor. Use the Parameter constants (e.g. p.MinValue) for convenience.
Does not support undo, unlike setRangeFromObject().

Dispatch/mechanics:
  RangeHelpers::isRangeId(id) validation -> data.setProperty(id, newValue, nullptr)
    -> rangeListener fires -> dynamicParameter->updateRange(data)

Pair with:
  getRangeObject -- read the full range
  setRangeFromObject -- bulk update with undo support

Anti-patterns:
  - Do NOT pass p.MidPoint -- silently ignored because MidPoint is not in the
    validated range ID set. Only MinValue, MaxValue, StepSize, SkewFactor accepted.
  - Do NOT rely on undo -- setRangeProperty passes nullptr to UndoManager.
    Use setRangeFromObject() when undo is needed.

Source:
  NodeBase.cpp  Parameter::setRangeProperty()
    -> RangeHelpers::isRangeId(id) check
    -> data.setProperty(id, newValue, nullptr)
