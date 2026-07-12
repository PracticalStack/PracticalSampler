ScriptedViewport::setTableColumns(Array columnMetadata) -> undefined

Thread safety: INIT
Defines the table columns. Must be called in onInit after setTableMode(). Each element is a JSON object with at minimum an ID property matching row data keys.

Supported common properties: `ID`, `Type`, `Label`, `Width`, `MinWidth`, `MaxWidth`, `Visible`, `Focus`.
Type-specific additions:
  - Button: `Toggle`, `Text`
  - Slider: `MinValue`, `MaxValue`, `StepSize`, `SkewFactor`, `suffix`, `defaultValue`, `showTextBox`, `style`
  - ComboBox: `items`, `ValueMode`, `Text`

For ComboBox columns, row data can override `items` and `Value` dynamically on a per-row basis.

Required setup:
  const var vp = Content.addViewport("ViewportId", 0, 0);
  vp.setTableMode({});
  vp.setTableColumns([
      {"ID": "Name", "Width": 150},
      {"ID": "Value", "Type": "Slider", "Width": 100}
  ]);
Dispatch/mechanics: Parses column metadata array. Creates internal column objects with type (Text/Button/Slider/ComboBox/Image/Hidden), width constraints, and type-specific settings. Configures TableListBox header columns.
Pair with: setTableMode (must be called first), setTableRowData (populates rows matching column IDs), setTableCallback (registers interaction handler)
Anti-patterns: Must be called in onInit -- reports a script error at runtime. Must call setTableMode() first. Column ID values must match property keys in row data objects.
Source:
  ScriptTableListModel.cpp  setColumns() -> iterates array -> creates ColumnMetadata -> configures TableHeaderComponent
