ScriptedViewport::setTableCallback(Function callbackFunction) -> undefined

Thread safety: INIT
Registers a callback for all user interactions with the table. Receives a JSON object with Type, rowIndex, columnID, and value properties. Must be called in onInit after setTableMode().
Callback signature: f(Object event)
Required setup:
  const var vp = Content.addViewport("ViewportId", 0, 0);
  vp.setTableMode({});
  vp.setTableColumns([{"ID": "Name", "Width": 150}]);
  inline function onTable(event) { /* ... */ };
  vp.setTableCallback(onTable);
Dispatch/mechanics: Stores callback as WeakCallbackHolder with 1 arg. On table interaction, builds DynamicObject with Type (event string), rowIndex, columnID, value. In MultiColumnMode, Selection and SingleClick are deferred to async update to coalesce rapid notifications.
Pair with: setTableMode (must be called first), setTableColumns (defines column structure), setTableRowData (populates rows), setEventTypesForValueCallback (controls which events trigger setValue)
Anti-patterns: Must be called in onInit -- reports a script error at runtime. Must call setTableMode() first. Use inline function syntax.
Source:
  ScriptTableListModel.cpp  setCallback() -> WeakCallbackHolder(1 arg)
  ScriptTableListModel.cpp  sendCallback() -> builds DynamicObject -> callSync()
