Parameter::getRangeObject() -> JSON

Thread safety: UNSAFE -- allocates a new DynamicObject on the heap.
Returns the parameter's range as a JSON snapshot with five properties: MinValue,
MaxValue, SkewFactor, StepSize, Inverted. Modifying the returned object does not
affect the parameter's range.

Pair with:
  setRangeFromObject -- apply a modified range object back to the parameter
  setRangeProperty -- set individual range properties

Source:
  NodeBase.cpp  Parameter::getRangeObject()
    -> creates DynamicObject with MinValue, MaxValue, SkewFactor, StepSize, Inverted
    -> reads from InvertableParameterRange stored in dynamicParameter
