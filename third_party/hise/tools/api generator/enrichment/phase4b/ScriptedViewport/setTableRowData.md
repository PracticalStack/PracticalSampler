ScriptedViewport::setTableRowData(Array tableData) -> undefined

Thread safety: UNSAFE
Updates row data for the table. Each array element is a JSON object with keys matching column ID values. Data is cloned internally -- original array preserved for getOriginalRowIndex() lookups. If sorting is active, new data is automatically re-sorted.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  vp.setTableRowData([
      {"Name": "Item 1", "Value": 0.5},
      {"Name": "Item 2", "Value": 0.8}
  ]);
Dispatch/mechanics: Clones input array. Stores clone as rowData and original reference as originalRowData. If sort function exists, re-sorts rowData. Triggers TableListBox update.
Pair with: setTableMode (must be called first), setTableColumns (defines what columns to display), getOriginalRowIndex (maps sorted index back to original)
Anti-patterns: Must call setTableMode() first -- reports a script error otherwise. Modifications to the original array after calling are not reflected in the table (data is cloned).
Source:
  ScriptTableListModel.cpp  setRowData() -> clone array -> optionally re-sort -> updateContent()
