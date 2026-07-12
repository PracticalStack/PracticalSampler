Table::getTablePointsAsArray() -> Array

Thread safety: WARNING -- acquires ScopedReadLock and iterates all graph points to build the return array. Lock-free but O(n) with heap allocation for array construction.
Returns all table control points as a nested array. Each element is [x, y, curve]
with normalized floats (0.0-1.0). The returned array is a snapshot -- modifying it
does not affect the table.

Pair with:
  setTablePointsFromArray -- inverse operation: set points from an array in the same format

Source:
  ScriptingApiObjects.cpp:2171  getTablePointsAsArray()
    -> Table::getTablePointsAsVarArray()
    -> ScopedReadLock(graphPointLock) -> iterate graphPoints -> [[x, y, curve], ...]
