TableProcessor::setTablePoint(int tableIndex, int pointIndex, Double x, Double y, Double curve) -> undefined

Thread safety: UNSAFE
Modifies an existing graph point. All values are clamped to 0..1 internally.
For edge points (first and last), only y and curve are updated -- x is
preserved at its fixed endpoint. For interior points, x, y, and curve are all
updated. Lookup table is recalculated and UI update is sent.

Required setup:
  const var tp = Synth.getTableProcessor("ModulatorId");

Dispatch/mechanics:
  ExternalDataHolder::getTable(tableIndex)
    -> Table::setTablePoint(pointIndex, x, y, curve)
    -> clamps all values to 0..1
    -> edge detection: if pointIndex == 0 or last, skip x assignment
    -> fillLookUpTable() + sendChangeMessage()

Pair with:
  addTablePoint -- add points first, then adjust with setTablePoint
  reset -- reset to default before rebuilding a curve

Anti-patterns:
  - Edge points (index 0 and last) silently ignore the x parameter. No error
    or warning is produced. Pass x conventionally (0.0 or 1.0) for clarity.
  - [BUG] Out-of-range pointIndex is silently ignored -- no error, no effect.
    The call appears to succeed but nothing changes.
  - Curve parameter: 0.5 = linear, < 0.5 = concave (faster initial change),
    > 0.5 = convex (slower initial change). This is not obvious from the
    signature alone.

Source:
  ScriptingApiObjects.h:2554  ScriptingTableProcessor::setTablePoint()
  Delegates to Table::setTablePoint() in hi_tools/hi_tools/Tables.h:50
