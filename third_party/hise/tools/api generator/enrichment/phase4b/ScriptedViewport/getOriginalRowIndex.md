ScriptedViewport::getOriginalRowIndex(Integer rowIndex) -> Integer

Thread safety: WARNING -- O(n) indexOf over row data; consider caching for large tables
Returns the index of the given row in the original (unsorted) data array from setTableRowData(). Maps from sorted display index back to original data order. Uses SimpleReadWriteLock for thread-safe access.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  var orig = vp.getOriginalRowIndex(0);
Dispatch/mechanics: Acquires SimpleReadWriteLock read lock on table model. Looks up item at rowIndex in sorted rowData, then finds that item in originalRowData via indexOf (object identity).
Pair with: setTableRowData (provides the original data), setTableSortFunction (enables sorting that makes this method necessary)
Anti-patterns: Do not call without calling setTableMode() first -- reports a script error. If two row objects are structurally identical, indexOf returns the first match.
Source:
  ScriptTableListModel.cpp  getOriginalRowIndex() -> SimpleReadWriteLock::ScopedReadLock -> originalRowData.indexOf()
