ScriptComboBox::addItem(String newName) -> undefined

Thread safety: UNSAFE
Appends a new item to the combo box's item list. Adds a newline and the item
name to the existing items property string, then increments max by 1.
Dispatch/mechanics:
  Reads current Items property string, appends "\n" + itemName (truncated to 128 chars)
    -> setScriptObjectProperty(Items, newItemList)
    -> increments max, resets min to 1
Pair with:
  getItemText -- retrieve display text of current selection
  set("items", ...) -- alternative: set entire item list at once
Anti-patterns:
  - Item names are silently truncated to 128 characters with no warning.
  - Do NOT call addItem() without clearing items first when rebuilding a list --
    stale items from the Interface Designer remain. Use set("items", "") first.
Source:
  ScriptingApiContent.cpp:3138  ScriptComboBox::addItem()
    -> appends to Items property, increments max
