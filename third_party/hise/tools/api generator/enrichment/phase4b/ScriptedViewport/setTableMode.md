ScriptedViewport::setTableMode(JSON tableMetadata) -> undefined

Thread safety: INIT
Activates table mode by creating the internal ScriptTableListModel. Must be called in onInit before setTableColumns() and setTableCallback(). Configures: RowHeight (default 20), HeaderHeight (default 24), Sortable (default false), MultiColumnMode (default false), MultiSelection (default false), ScrollOnDrag (default false), ProcessSpaceKey (default false), CallbackOnSliderDrag (default true), SliderRangeIdSet (default "scriptnode").
Required setup:
  const var vp = Content.addViewport("ViewportId", 0, 0);
  vp.setTableMode({"RowHeight": 28, "Sortable": true});
Dispatch/mechanics: Creates ScriptTableListModel, stores as tableModel member. Parses metadata JSON for table-wide configuration. Mode is permanent for this compilation -- cannot switch back.
Pair with: setTableColumns (defines column structure, call next), setTableRowData (populates rows), setTableCallback (registers interaction handler), setTableSortFunction (custom sort comparator), setEventTypesForValueCallback (controls value callback events)
Anti-patterns: Must be called in onInit -- reports a script error at runtime. Once called, viewport is permanently in table mode until recompilation.
Source:
  ScriptingApiContent.cpp:5483  ScriptedViewport::setTableMode() -> new ScriptTableListModel()
