Table::reset() -> undefined

Thread safety: UNSAFE -- acquires ScopedWriteLock, clears graphPoints array (heap deallocation), rebuilds lookup table (HeapBlock allocation + Path construction).
Resets the table to its default state: two control points at (0, 0, 0.5) and
(1, 1, 0.5), producing a linear ramp from 0 to 1. Sends a ContentChange notification
with point index -1 (bulk change).

Dispatch/mechanics:
  ScopedWriteLock(graphPointLock) -> graphPoints.clear()
    -> add (0, 0, 0.5) and (1, 1, 0.5)
    -> sendContentChangeMessage(-1, sendNotificationAsync)
    -> fillLookUpTable()

Pair with:
  setContentCallback -- fires with index -1 after reset
  setTablePointsFromArray -- alternative for setting a specific curve instead of default

Source:
  Tables.cpp:142  Table::reset()
    -> ScopedWriteLock(graphPointLock)
    -> graphPoints.clearQuick(), add two default points
    -> fillLookUpTable()
