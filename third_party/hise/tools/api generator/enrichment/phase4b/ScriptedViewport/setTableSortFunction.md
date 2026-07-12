ScriptedViewport::setTableSortFunction(Function sortFunction) -> undefined

Thread safety: UNSAFE
Sets a custom comparator for column header click-to-sort (requires Sortable: true in table metadata). Receives two cell values from the sort column, must return negative/0/positive integer. Passing a non-function reverts to default alphabetic/numeric sorter.
Callback signature: f(var a, var b)
Required setup:
  const var vp = Content.getComponent("ViewportId");
  inline function mySort(a, b) { return a < b ? -1 : (a > b ? 1 : 0); };
  vp.setTableSortFunction(mySort);
Dispatch/mechanics: Stores sort function reference. Called synchronously via callSync when user clicks a column header. Receives individual cell values from the sort column, not full row objects.
Pair with: setTableMode (must be called first with Sortable: true), getOriginalRowIndex (maps sorted indices back to original order)
Anti-patterns: Must call setTableMode() first -- reports a script error otherwise. Sort function is called synchronously -- must not perform long-running operations.
Source:
  ScriptTableListModel.cpp  setSortFunction() -> stores WeakCallbackHolder
  ScriptTableListModel.cpp  sortOrderChanged() -> callSync(a, b)
