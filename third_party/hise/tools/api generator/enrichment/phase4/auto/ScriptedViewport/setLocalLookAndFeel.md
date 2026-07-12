Attaches a scripted look and feel object to this component and all its children. Pass `false` to clear it. The relevant LAF functions are `drawTableRowBackground`, `drawTableCell`, `drawTableHeaderBackground`, `drawTableHeaderColumn`, and `drawScrollbar`.

> [!Warning:sortColumnId is 1-based, columnIndex 0-based] The `obj.sortColumnId` in `drawTableHeaderColumn` is 1-based (matching JUCE's `TableHeaderComponent` convention), while `obj.columnIndex` is 0-based. Compare them as `obj.sortColumnId == (obj.columnIndex + 1)` to detect the active sort column.
