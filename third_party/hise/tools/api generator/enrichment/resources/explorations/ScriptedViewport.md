# ScriptedViewport -- Raw Exploration

## Resources Consulted
- `resources/explorations/ScriptComponent_base.md` (ScriptComponent base class)

## Source Files
- **Header:** `hi_scripting/scripting/api/ScriptingApiContent.h` (lines 2070-2160)
- **Implementation:** `hi_scripting/scripting/api/ScriptingApiContent.cpp` (lines 5483-5802)
- **Table model header:** `hi_scripting/scripting/api/ScriptTableListModel.h`
- **Table model impl:** `hi_scripting/scripting/api/ScriptTableListModel.cpp`
- **Wrapper:** `hi_scripting/scripting/api/ScriptComponentWrappers.h` (lines 760-923)
- **Wrapper impl:** `hi_scripting/scripting/api/ScriptComponentWrappers.cpp` (lines 1762-2103)

## Class Declaration

```cpp
struct ScriptedViewport : public ScriptComponent
{
public:
    enum Properties
    {
        scrollbarThickness = ScriptComponent::numProperties,
        autoHide,
        useList,
        viewPositionX,
        viewPositionY,
        Items,
        FontName,
        FontSize,
        FontStyle,
        Alignment,
        numProperties
    };

    // Inherits ScriptComponent constructor, adds 7 own API methods
    // Overrides: setValue, getValue (via base virtual)
    // Deactivates: macroControl, min, max

    ScriptTableListModel::Ptr getTableModel() { return tableModel; }
    LambdaBroadcaster<double, double> positionBroadcaster;

private:
    ScriptTableListModel::Ptr tableModel;
    StringArray currentItems;
};
```

## Constructor -- Properties and API Registration

No `addConstant()` calls. No `ADD_TYPED_API_METHOD_N` calls. All 7 API methods use plain `ADD_API_METHOD_1`.

### Properties

| Enum | Property ID | Type | Default | Range |
|---|---|---|---|---|
| scrollbarThickness | "scrollBarThickness" | Slider | 16.0 | 0-40, step 1 |
| autoHide | "autoHide" | Toggle | true | -- |
| useList | "useList" | Toggle | false | -- |
| viewPositionX | "viewPositionX" | Slider | 0.0 | 0-1, step 0.01 |
| viewPositionY | "viewPositionY" | Slider | 0.0 | 0-1, step 0.01 |
| Items | "items" | Multiline | "" | -- |
| FontName | "fontName" | Choice | "Arial" | -- |
| FontSize | "fontSize" | Slider | 13.0 | 1-200, step 1 |
| FontStyle | "fontStyle" | Choice | "plain" | -- |
| Alignment | "alignment" | Choice | "centred" | -- |

### Deactivated Properties
- macroControl (in constructor)
- min, max (in handleDefaultDeactivatedProperties)

### Registered API Methods (all ADD_API_METHOD_1)
| Method | Returns |
|---|---|
| setTableMode | void |
| setTableColumns | void |
| setTableRowData | void |
| setTableCallback | void |
| getOriginalRowIndex | int |
| setTableSortFunction | void |
| setEventTypesForValueCallback | void |

## Three Viewport Modes

```
Mode enum: List, Table, Viewport

Determination:
  if (getTableModel() != null) => Table
  else if (useList == true) => List
  else => Viewport
```

| Mode | Condition | JUCE Component | Model |
|---|---|---|---|
| Table | setTableMode() called in onInit | TableListBox | ScriptTableListModel |
| List | useList=true, no table model | ListBox | ColumnListBoxModel (string items from Items property) |
| Viewport | useList=false, no table model | Viewport with DummyComponent(4000x4000) | None (pure scroll) |

## setValue Override

```cpp
void ScriptedViewport::setValue(var newValue)
{
    if (tableModel != nullptr)
    {
        if (newValue.isArray() && newValue.size() == 2)
        {
            auto c = (int)newValue[0];
            auto r = (int)newValue[1];
            auto useUndo = (bool)getScriptObjectProperty(useUndoManager);

            ScopedPointer<UndoableAction> u = new UndoableTableSelection(this, c, r);

            if (useUndo)
                getScriptProcessor()->getMainController_()->getControlUndoManager()->perform(u.release());
            else
                u->perform();
        }
    }
    ScriptComponent::setValue(newValue);
}
```

In table mode with MultiColumnMode, accepts [column, row] array. Wraps in UndoableTableSelection. Always calls base setValue.
In list mode, accepts integer row index (set by ColumnListBoxModel on click/return).
In viewport mode, accepts arbitrary value (position tracking is via properties).

## getValue Override

ScriptedViewport does NOT override getValue -- it uses the base ScriptComponent::getValue(). The base exploration notes it as overridden, but looking at the header, only setValue is overridden. The value stored depends on mode:
- List mode: integer row index
- Table mode (MultiColumnMode): [column, row] array
- Viewport mode: whatever was set via setValue

## ScriptTableListModel -- EventType Enum

```cpp
enum class EventType
{
    SliderCallback,    // 0
    ButtonCallback,    // 1
    ComboboxCallback,  // 2
    Selection,         // 3
    SingleClick,       // 4
    DoubleClick,       // 5
    ReturnKey,         // 6
    SpaceKey,          // 7
    SetValue,          // 8
    Undo,              // 9
    DeleteRow,         // 10
    numEventTypes
};
```

## ScriptTableListModel -- CellType Enum

```cpp
enum class CellType { Text, Button, Image, Slider, ComboBox, Hidden, numCellTypes };
```

## Table Callback Object (sendCallback)

The DynamicObject passed to setTableCallback function:

| Property | Type | Description |
|---|---|---|
| Type | String | Event type string (see mapping below) |
| rowIndex | int | Row index, -1 for background click |
| columnID | var | The ID property from column metadata (if valid column) |
| value | var | Depends on event type |

### EventType to "Type" String Mapping

| EventType | "Type" string |
|---|---|
| SliderCallback | "Slider" |
| ButtonCallback | "Button" |
| ComboboxCallback | "ComboBox" |
| Selection | "Selection" |
| SingleClick | "Click" |
| DoubleClick | "DoubleClick" |
| ReturnKey | "ReturnKey" |
| DeleteRow | "DeleteRow" |
| SetValue | "SetValue" |
| Undo | "SpaceKey" (bug: fallthrough overwrites "Undo") |
| SpaceKey | "SpaceKey" |

### Value content by event type
- SingleClick, DoubleClick, Selection, ReturnKey, DeleteRow: full row data object
- SliderCallback: slider double value
- ButtonCallback: toggle state bool
- ComboboxCallback: depends on ValueMode (id/index/text)
- SetValue, Undo: full row data object (looked up from rowData)
- backgroundClicked: empty array

## Table Metadata (setTableMode parameter)

| Property | Type | Default | Description |
|---|---|---|---|
| MultiColumnMode | bool | false | Enables [column, row] value tracking |
| Sortable | bool | false | Column header sort clicking |
| HeaderHeight | int | 24 | Header height in pixels |
| RowHeight | int | 20 | Row height in pixels |
| MultiSelection | bool | false | Multiple row selection |
| ScrollOnDrag | bool | false | Touch scroll-on-drag |
| ProcessSpaceKey | bool | false | SpaceKey event handling |
| CallbackOnSliderDrag | bool | true | Slider callback during drag vs on release |
| SliderRangeIdSet | String | "scriptnode" | Range ID set: "scriptnode", "ScriptComponent", "MidiAutomation", "MidiAutomationFull" |

## Column Metadata (setTableColumns parameter)

Array of JSON objects, each with:

| Property | Type | Default | Description |
|---|---|---|---|
| ID | String | (required) | Column identifier, matches row data keys |
| Label | String | same as ID | Display name in header |
| Type | String | "Text" | Cell type: "Text", "Button", "Image", "Slider", "ComboBox", "Hidden" |
| Width | int | -- | Column width in pixels |
| MinWidth | int | 1 | Minimum column width |
| MaxWidth | int | -1 (unlimited) | Maximum column width |
| Visible | bool | true | Whether column is visible |
| PeriodicRepaint | bool | false | Timer-based column repaint |
| Focus | bool | true | Keyboard focus for arrow navigation |

Button-specific: Text (default "Button"), Toggle (default false)
ComboBox-specific: Text (default "No selection"), ValueMode ("ID", "Index", "Text")
Slider-specific: suffix, defaultValue, showTextBox (true), style ("Knob", "Horizontal", "Vertical"), plus range properties

## Row Data Format (setTableRowData parameter)

Array of JSON objects. Keys match column ID values. Example:
```javascript
[
    { "Name": "Item 1", "Value": 0.5, "Enabled": true },
    { "Name": "Item 2", "Value": 0.8, "Enabled": false }
]
```

## setEventTypesForValueCallback

Takes array of event type name strings. Controls which events trigger the parent setValue/additionalCallback.

Valid strings: "SliderCallback", "ButtonCallback", "Selection", "SingleClick", "DoubleClick", "ReturnKey", "SetValue", "Undo", "DeleteRow"
Illegal for value callback: "SliderCallback", "ButtonCallback", "SetValue", "Undo", "DeleteRow"
Legal for value callback: "Selection", "SingleClick", "DoubleClick", "ReturnKey"
Defaults (set in constructor): SingleClick, DoubleClick, ReturnKey, SpaceKey

NOTE: There is an enum index mismatch bug in this method. The string array skips "ComboboxCallback" and "SpaceKey", so the (EventType)idx cast maps string indices to wrong enum values. "Selection" at index 2 maps to EventType::ComboboxCallback (enum 2) instead of EventType::Selection (enum 3).

## setTableSortFunction

Custom sort function receives 2 args (cell values being compared from the sort column). Must return integer: negative if v1 < v2, positive if v1 > v2, 0 if equal. Called synchronously via callSync. Passing non-function reverts to default sorter.

## getOriginalRowIndex

Looks up the item at rowIndex in the (possibly sorted) rowData, then finds that same item in originalRowData via indexOf. Thread-safe with SimpleReadWriteLock.

## setTableCallback

Must be called in onInit. Callback receives 1 argument (DynamicObject with Type, rowIndex, columnID, value). Uses WeakCallbackHolder with 1 arg.

## Property Change Handling

- Items property change: parses newline-separated string into currentItems StringArray
- viewPositionX/Y change: broadcasts via positionBroadcaster (LambdaBroadcaster<double,double>)

## List Mode (ColumnListBoxModel)

- Items from Items property (newline-separated string)
- Click/ReturnKey calls setValue(rowIndex) + changed()
- Row height fixed at 30px
- Single selection only
- Supports CSS styling via StyleSheetLookAndFeel::drawListBoxRow
- Colours: bgColour, itemColour (selected bg), itemColour2 (selected outline), textColour

## Viewport Mode

- Creates a Viewport with DummyComponent(4000x4000) for scrolling
- Scroll positions (viewPositionX, viewPositionY) normalized 0.0-1.0
- positionBroadcaster syncs property values with viewport scroll position

## LookAndFeel Functions (Table/List mode)

Table mode: drawTableRowBackground, drawTableCell, drawTableHeaderBackground, drawTableHeaderColumn
List mode: drawListBoxRow (via CSS StyleSheetLookAndFeel)
All modes: drawScrollbar

## UndoableTableSelection

In MultiColumnMode, setValue([column, row]) creates an UndoableTableSelection that:
- perform() calls sendCallback(newRow, newColumn+1, true, EventType::SetValue)
- undo() calls sendCallback(oldRow, oldColumn+1, true, EventType::Undo)
Both return false (JUCE convention: action not actually tracked by UndoManager).

## handleDefaultDeactivatedProperties

Deactivates min and max properties (viewport does not use numeric ranges).

## Factory Method (obtainedVia)

`Content.addViewport(name, x, y)` -- accepts 1 or 3 args.
