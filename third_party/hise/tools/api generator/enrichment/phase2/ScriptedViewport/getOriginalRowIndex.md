## getOriginalRowIndex

**Examples:**

```javascript:mapping-sorted-display-index
// Title: Mapping sorted display index back to data for favorite toggle
// Context: When the table is sorted by a column header, the display row
//          indices no longer match the original data array. Use
//          getOriginalRowIndex() to find the correct position in the
//          original data before mutating it.

const var table = Content.addViewport("DataTable", 0, 0);
table.setTableMode({ "RowHeight": 30, "Sortable": true, "HeaderHeight": 28 });

table.setTableColumns([
    { "ID": "Favorite", "Type": "Button", "MinWidth": 32,
      "Toggle": true,   "Focus": false, "Text": "Fav" },
    { "ID": "Name",     "Label": "Name", "MinWidth": 200 }
]);

var originalData = [
    {"Favorite": 0, "Name": "Item A", "isFavorite": false},
    {"Favorite": 1, "Name": "Item B", "isFavorite": true},
    {"Favorite": 0, "Name": "Item C", "isFavorite": false}
];

table.setTableRowData(originalData);

// Sort by Name descending so display order differs from data order
// After sort: display 0 = Item C (original 2), display 2 = Item A (original 0)
table.setTableSortFunction(function(a, b) { return a.Name > b.Name; });

inline function onTableEvent(event)
{
    if (event.columnID == "Favorite")
    {
        // Map display row to original data index
        local dataIndex = table.getOriginalRowIndex(event.rowIndex);
        originalData[dataIndex].isFavorite = event.value;
        Console.print("Toggled favorite at original index: " + dataIndex);
    }
};

table.setTableCallback(onTableEvent);
```
```json:testMetadata:mapping-sorted-display-index
{
  "testable": true,
  "skipReason": "Sort requires UI column-header click to trigger; setTableSortFunction only registers comparator"
}
```


```javascript:selecting-the-current-item
// Title: Selecting an item by original data index in a sorted table
// Context: After sorting, display indices no longer match data indices.
//          Walk the display rows with getOriginalRowIndex() to find which
//          one corresponds to a known data index, then select it.

const var sortedTable = Content.addViewport("SortedTable", 0, 0);
sortedTable.setTableMode({"RowHeight": 30, "Sortable": true, "HeaderHeight": 28});
sortedTable.setTableColumns([{"ID": "Name", "Label": "Name", "MinWidth": 200}]);

var fruits = [
    {"Name": "Cherry"},
    {"Name": "Apple"},
    {"Name": "Banana"}
];

sortedTable.setTableRowData(fruits);

// Sort alphabetically: display 0 = Apple (orig 1), 1 = Banana (orig 2), 2 = Cherry (orig 0)
sortedTable.setTableSortFunction(function(a, b) { return a.Name < b.Name; });

inline function selectByDataIndex(dataIndex)
{
    for (i = 0; i < fruits.length; i++)
    {
        if (sortedTable.getOriginalRowIndex(i) == dataIndex)
        {
            sortedTable.set("saveInPreset", false);
            sortedTable.setValue(i);
            return i;
        }
    }
    return -1;
}

// Select "Cherry" which is original index 0, now at display index 2
var displayIndex = selectByDataIndex(0);
```
```json:testMetadata:selecting-the-current-item
{
  "testable": true,
  "skipReason": "Sort requires UI column-header click to trigger; setTableSortFunction only registers comparator"
}
```

**Pitfalls:**
- This method performs an O(n) search internally. For large tables (thousands of rows), avoid calling it in a tight loop. The pattern of walking all display rows and comparing `getOriginalRowIndex(i) == targetIndex` is O(n^2) in the worst case -- cache the mapping if called frequently.
