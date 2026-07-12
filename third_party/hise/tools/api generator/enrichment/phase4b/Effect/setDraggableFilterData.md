Effect::setDraggableFilterData(JSON filterData) -> undefined

Thread safety: UNSAFE -- modifies internal processor filter statistics state.
Configures the draggable filter visualization for effects implementing
ProcessorWithCustomFilterStatistics (Script FX, Hardcoded FX, Polyphonic
Filter). Silently does nothing on unsupported effects.

filterData schema:
  NumFilterBands: int          -- number of filter bands
  FilterDataSlot: int          -- ExternalData FilterCoefficients slot index
  FirstBandOffset: int         -- attribute index where band parameters start
  TypeList: Array              -- filter type display names
  ParameterOrder: Array        -- parameter names per band in attribute order
  FFTDisplayBufferIndex: int   -- display buffer index for FFT (-1 to disable)
  DragActions.DragX: String    -- parameter for horizontal drag (e.g., "Freq")
  DragActions.DragY: String    -- parameter for vertical drag (e.g., "Gain")
  DragActions.ShiftDrag: String -- parameter for shift+drag (e.g., "Q")
  DragActions.DoubleClick: String -- parameter toggled on double-click
  DragActions.RightClick: String  -- parameter for right-click ("" for none)

Dispatch/mechanics:
  dynamic_cast<ProcessorWithCustomFilterStatistics*>(getEffect())
    -> h->setFilterStatistics(filterData)
Pair with:
  getDraggableFilterData -- read back current configuration
Anti-patterns:
  - Do NOT assume all effects support this -- silently does nothing for effects
    without ProcessorWithCustomFilterStatistics. No error is reported.
Source:
  ScriptingApiObjects.cpp:3672  ScriptingEffect::setDraggableFilterData()
    -> dynamic_cast<ProcessorWithCustomFilterStatistics*> -> setFilterStatistics(filterData)
