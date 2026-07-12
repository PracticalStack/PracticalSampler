Effect::getDraggableFilterData() -> JSON

Thread safety: UNSAFE -- creates a DynamicObject on the heap for the JSON return value.
Returns the current draggable filter configuration as a JSON object for effects
implementing ProcessorWithCustomFilterStatistics (Script FX, Hardcoded FX,
Polyphonic Filter). Returns undefined for unsupported effects.
Dispatch/mechanics:
  dynamic_cast<ProcessorWithCustomFilterStatistics*>(getEffect())
    -> h->getFilterStatisticsJSON()
    -> returns DynamicObject with NumFilterBands, ParameterOrder, DragActions, etc.
Pair with:
  setDraggableFilterData -- configure the filter visualization
Anti-patterns:
  - Do NOT assume all effects support this -- silently returns undefined for
    effects without ProcessorWithCustomFilterStatistics. Check return value.
Source:
  ScriptingApiObjects.cpp:3672  ScriptingEffect::getDraggableFilterData()
    -> dynamic_cast<ProcessorWithCustomFilterStatistics*> -> getFilterStatisticsJSON()
