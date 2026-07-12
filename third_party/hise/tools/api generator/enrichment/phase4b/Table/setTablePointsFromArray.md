Table::setTablePointsFromArray(Array pointList) -> undefined

Thread safety: UNSAFE -- acquires ScopedWriteLock, replaces all graph points, calls fillLookUpTable() which allocates a HeapBlock and creates a JUCE Path.
Replaces all table control points from a nested array. Each element must be a
3-element sub-array [x, y, curve] with normalized floats. Requires at least 2 points
(script error otherwise). All values are clamped to 0.0-1.0. First point x is forced
to 0.0, last point x to 1.0. Points are sorted by x, lookup table is rendered once.

Required setup:
  const var td = Engine.createAndRegisterTableData(0);

Dispatch/mechanics:
  validates each sub-array has 3 elements -> jlimit clamp all values
    -> forces edge x positions (first=0.0, last=1.0)
    -> Table::setGraphPoints() -> sort by x -> fillLookUpTable() once
    -> sendContentChangeMessage(-1, sendNotificationAsync)

Pair with:
  getTablePointsAsArray -- inverse operation: retrieve points in the same format
  reset -- shorthand for restoring the default linear ramp

Anti-patterns:
  - Do NOT pass fewer than 2 points -- triggers a script error.
  - First point x is silently forced to 0.0, last point x to 1.0 regardless of values
    passed. Do not rely on edge point x positions being preserved.

Source:
  ScriptingApiObjects.cpp:2181  ScriptTableData::setTablePointsFromArray()
    -> validates sub-array size (reportScriptError if != 3)
    -> jlimit(0.0f, 1.0f, ...) all values
    -> forces edge x: points[0].x = 0.0, points[last].x = 1.0
    -> Table::setGraphPoints() -> sort -> fillLookUpTable()
