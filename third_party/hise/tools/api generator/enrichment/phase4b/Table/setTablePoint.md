Table::setTablePoint(Number pointIndex, Number x, Number y, Number curve) -> undefined

Thread safety: UNSAFE -- acquires ScopedReadLock on graphPoints, then calls fillLookUpTable() which allocates a HeapBlock and creates a JUCE Path.
Modifies an existing control point at the given index. All values are clamped to
0.0-1.0 via jlimit. For edge points (index 0 and last), x is preserved -- only y
and curve are updated. After modification, the lookup table is re-rendered and a
synchronous ContentChange notification is sent.

Pair with:
  getTablePointsAsArray -- retrieve current points to find indices
  setTablePointsFromArray -- bulk alternative for modifying all points at once

Anti-patterns:
  - Edge points (first and last) silently ignore the x parameter -- only y and curve
    are applied. No error is thrown for a different x value on an edge point.
  - Each call triggers a full lookup table re-render. For modifying multiple points,
    use setTablePointsFromArray() which renders only once.

Source:
  Tables.cpp:113  Table::setTablePoint()
    -> jlimit(0.0f, 1.0f, ...) for all values
    -> ScopedReadLock(graphPointLock)
    -> edge check: if (index == 0 || index == last) preserve x
    -> fillLookUpTable() -> sendContentChangeMessage(sendNotificationSync)
