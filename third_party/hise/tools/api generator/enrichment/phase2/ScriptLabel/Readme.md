# ScriptLabel -- Project Context

## Project Context

### Real-World Use Cases
- **Search and filter inputs**: Editable labels used as lightweight text fields for filtering lists or tables; `updateEachKey` drives live results while typing.
- **Inline naming or renaming**: Labels used as small text inputs for naming items or saving user-entered strings without a custom dialog.
- **Static UI annotations**: Read-only labels for headers, parameter group names, or overlay text in a panel.

### Complexity Tiers
1. **Static display**: `set("text", ...)`, `set("fontName", ...)`, `set("alignment", ...)` for layout-only labels.
2. **Editable input**: `setEditable()` in `onInit`, `setControlCallback()` to react to user edits.
3. **Live search field**: `set("updateEachKey", true)` plus `setControlCallback()` and `changed()` for programmatic clears.

### Practical Defaults
- Use `set("updateEachKey", true)` only for live-search or incremental filtering; keep it false for normal labels to avoid extra callbacks.
- `set("saveInPreset", false)` is a good default for transient UI inputs like search fields; keep it true only when the label represents user state that should persist.
- `set("alignment", "left")` is a good default for editable text fields.

### Integration Patterns
- `ScriptLabel.setControlCallback()` -> `ScriptedViewport.setTableRowData()` -- rebuild list data when the search field changes.
- `ScriptLabel.changed()` -> `Broadcaster.resendLastMessage()` -- reuse the last refresh message after programmatic text updates.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Set label text programmatically and expect callbacks to fire | Call `changed()` after updating the text | Programmatic text updates do not trigger `onControl` unless `changed()` is called. |
