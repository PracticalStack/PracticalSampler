# showAsPopup | UNSAFE

Shows this panel as a popup overlay on top of the interface. If `closeOtherPopups` is true (1), all other currently visible popup panels are closed first. The `isPopupPanel` property should be set to `true` for proper popup behavior (hidden until shown).

```
showAsPopup(int closeOtherPopups)
```

## Dispatch / Mechanics

Calls `parent->addPanelPopup(this, closeOther)` which manages popup panels in the Content's `popupPanels` array.

## Required Setup

Set the `isPopupPanel` property to `true`. This keeps the panel hidden until `showAsPopup()` is called. The panel's `isShowing()` override returns `false` while the popup is not visible, preventing unnecessary paint routine execution.

## Pair With

- `closeAsPopup()` - hide the popup
- `isVisibleAsPopup()` - check current popup visibility
- `setIsModalPopup()` - make the popup modal with dark background
- `setPopupData()` - configure FloatingTile content and bounds

## Source

`ScriptingApiContent.cpp` line ~4220
