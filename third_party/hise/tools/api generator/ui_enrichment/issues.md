# UI Component Issues

Bugs and implementation issues found during forum enrichment. These should be fixed in code, not documented as workarounds.

---

## ScriptedViewport

### Table mode Slider columns do not visually update

**Status:** Open
**Source:** Forum topic 6026
**Verified:** Yes - ScriptTableListModel.cpp:297 updates slider value via `ComponentUpdateHelpers::updateValue()` but no repaint/invalidation follows. Lines 385-399 show the callback fires correctly.

Slider-type columns in table mode fire the table callback with the correct new value, but the slider thumb does not visually move. The value change is processed internally but the table row is not repainted. A custom LAF with `drawTableCell` can work around this by forcing redraws, but the proper fix would add a row repaint after slider value changes.
