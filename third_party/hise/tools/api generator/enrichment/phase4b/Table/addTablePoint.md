Table::addTablePoint(Number x, Number y) -> undefined

Thread safety: UNSAFE -- acquires ScopedWriteLock on graphPoints, then calls fillLookUpTable() which allocates a HeapBlock and creates a JUCE Path.
Adds a new control point at the given normalized coordinates. Curve factor defaults to 0.5 (linear).
After adding, the 512-element lookup table is re-rendered and a ContentChange notification is sent asynchronously.

Dispatch/mechanics:
  ScopedWriteLock(graphPointLock) -> graphPoints.add({x, y, 0.5f})
    -> sendContentChangeMessage(sendNotificationAsync)
    -> fillLookUpTable(): Path construction + PathFlatteningIterator -> 512 floats

Pair with:
  setTablePointsFromArray -- bulk alternative that renders lookup table only once
  getTablePointsAsArray -- retrieve current points after adding

Anti-patterns:
  - Do NOT call addTablePoint() in a loop for bulk point setup -- each call triggers a
    full lookup table re-render (512 floats via PathFlatteningIterator). Use
    setTablePointsFromArray() instead.

Source:
  Tables.cpp:158  Table::addTablePoint()
    -> ScopedWriteLock(graphPointLock)
    -> graphPoints.add({x, y, 0.5f})
    -> fillLookUpTable() -> fillExternalLookupTable() -> Path + PathFlatteningIterator
