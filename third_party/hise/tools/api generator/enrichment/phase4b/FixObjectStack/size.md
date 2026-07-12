FixObjectStack::size() -> Integer

Thread safety: SAFE
Returns the number of used elements (the position pointer value). This is NOT the
allocated capacity -- use the length constant for that.

Source:
  FixLayoutObjects.cpp:1330  Stack::size()
    -> return position (overrides Array::size which returns numElements)
