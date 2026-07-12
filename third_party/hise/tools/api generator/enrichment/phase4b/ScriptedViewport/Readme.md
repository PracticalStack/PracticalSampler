ScriptedViewport (object)
Obtain via: Content.addViewport(name, x, y)

Multi-mode viewport component supporting scrollable viewport, selectable list,
and interactive table modes. Mode is determined at init time: default is a plain
scrollable viewport, useList property enables list mode, setTableMode() enables
table mode with sortable columns and editable cells.

Use Viewport mode when you only need scrolling, List mode for simple item selection, and Table mode when each row has multiple columns or interactive cells.

Complexity tiers:
  1. Plain viewport: Content.addViewport, set, showControl. Scrollable container
     for child panels. Only drawScrollbar LAF and scrollBarThickness needed.
  2. Simple table: + setTableMode, setTableColumns, setTableCallback,
     setTableRowData. Text and Button column types with a callback that switches
     on event.Type.
  3. Interactive table with sorting: + setTableSortFunction,
     getOriginalRowIndex, setEventTypesForValueCallback. Enable Sortable,
     MultiColumnMode, MultiSelection. Add Slider/ComboBox columns. Customize
     all four table LAF functions.

Practical defaults:
  - Use "RowHeight": 28 to 33 for comfortable touch and mouse targets.
  - Use "HeaderHeight": 0 when column labels are not needed (simple selectors).
  - Use "ScrollOnDrag": true for any table used on touchscreens or pen tablets.
  - Use "MultiColumnMode": true when you need setValue([column, row]) for
    programmatic cell selection in a sorted table.
  - Set scrollBarThickness to 8-10 pixels for a minimal scrollbar with custom
    drawScrollbar LAF.
  - Pass a Broadcaster to setTableCallback() instead of an inline function when
    table events need to drive multiple listeners.

Table setup sequence:

![Table Mode Setup Sequence](sequence_table-setup.svg)

All table setup must happen during onInit. After that, update only the row data and table state.

Common mistakes:
  - Using obj.rowIndex to index data when sorting is enabled -- display indices
    differ from data indices. Use getOriginalRowIndex(obj.rowIndex) instead.
  - Calling setTableMode() or setTableColumns() outside onInit -- reports a
    script error. Both must be called in onInit.
  - Calling setTableColumns() without calling setTableMode() first -- the
    internal table model does not exist yet.
  - Using a regular function for setTableCallback -- must use inline function.
  - Setting viewPositionY to a pixel value -- the property is normalized 0.0-1.0.
    Use (targetRow + 1) / totalRows to scroll to a specific row.

Example:
  // Table mode: define columns and populate rows
  const var Viewport1 = Content.addViewport("Viewport1", 0, 0);

  Viewport1.setTableMode({ "RowHeight": 30, "HeaderHeight": 28 });

  Viewport1.setTableColumns([
      { "ID": "Name", "Label": "Name", "Width": 150 },
      { "ID": "Value", "Label": "Value", "Type": "Slider", "Width": 100 }
  ]);

  Viewport1.setTableCallback(inline function(event)
  {
      Console.print(event.Type + " row: " + event.rowIndex);
  });

  Viewport1.setTableRowData([
      { "Name": "Item A", "Value": 0.5 },
      { "Name": "Item B", "Value": 0.8 }
  ]);

Methods (37):
  changed                        fadeComponent
  get                            getAllProperties
  getChildComponents             getGlobalPositionX
  getGlobalPositionY             getHeight
  getId                          getLocalBounds
  getOriginalRowIndex            getValue
  getWidth                       grabFocus
  loseFocus                      sendRepaintMessage
  set                            setConsumedKeyPresses
  setControlCallback             setEventTypesForValueCallback
  setKeyPressCallback            setLocalLookAndFeel
  setPosition                    setStyleSheetClass
  setStyleSheetProperty          setStyleSheetPseudoState
  setTableCallback               setTableColumns
  setTableMode                   setTableRowData
  setTableSortFunction           setTooltip
  setValue                       setValueWithUndo
  setZLevel                      showControl
  updateValueFromProcessorConnection
