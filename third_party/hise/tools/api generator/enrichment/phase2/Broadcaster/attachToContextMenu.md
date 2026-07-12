## attachToContextMenu

**Examples:**

```javascript:context-menu-dynamic-state
// Title: Context menu with dynamic state and tick marks
// Context: Right-click menus with checkmarks that reflect application state.
// The state function is called before the menu opens to set tick marks
// and enabled/disabled states for each item.

// --- setup ---
const var OptionsPanel = Content.addPanel("OptionsPanel", 0, 0);
OptionsPanel.set("width", 200);
OptionsPanel.set("height", 200);
OptionsPanel.set("saveInPreset", false);
// --- end setup ---

const var menuBc = Engine.createBroadcaster({
    "id": "OptionsMenu",
    "args": ["component", "menuItemIndex"]
});

reg quantizeEnabled = 1;
reg metronomeEnabled = 0;

inline function menuState(type, index)
{
    if (type == "active")
    {
        // Show tick marks for enabled options
        if (index == 1) return quantizeEnabled;
        if (index == 2) return metronomeEnabled;
    }

    if (type == "enabled")
        return true;

    return "";
}

inline function onMenuSelect(component, menuItemIndex)
{
    if (menuItemIndex == 1)
        quantizeEnabled = !quantizeEnabled;

    if (menuItemIndex == 2)
        metronomeEnabled = !metronomeEnabled;
}

menuBc.addListener("", "handleMenu", onMenuSelect);

// Headers use "**Bold Text**" syntax and are not selectable
menuBc.attachToContextMenu(
    OptionsPanel,
    menuState,
    ["**Recording Options**", "Quantize to grid", "Enable metronome"],
    "recordMenu",
    false  // false = right-click, true = left-click
);
```
```json:testMetadata:context-menu-dynamic-state
{
  "testable": false,
  "skipReason": "Context menu requires physical user interaction (right-click) that cannot be triggered programmatically from script"
}
```

```javascript:left-click-context-menu
// Title: Left-click context menu on multiple components
// Context: A set of buttons that open a shared context menu on left-click
// rather than right-click. Useful for "more options" buttons.

// --- setup ---
const var MoreBtn1 = Content.addButton("MoreBtn1", 0, 0);
MoreBtn1.set("saveInPreset", false);
const var MoreBtn2 = Content.addButton("MoreBtn2", 130, 0);
MoreBtn2.set("saveInPreset", false);
const var MoreBtn3 = Content.addButton("MoreBtn3", 260, 0);
MoreBtn3.set("saveInPreset", false);
// --- end setup ---

const var moreBc = Engine.createBroadcaster({
    "id": "MoreOptions",
    "args": ["component", "menuItemIndex"]
});

moreBc.attachToContextMenu(
    [MoreBtn1, MoreBtn2, MoreBtn3],
    false,  // No state function - all items always enabled, no ticks
    ["Copy", "Paste", "Reset to default"],
    "moreMenu",
    true  // Left-click opens the menu
);

var menuSelections = [];

moreBc.addListener("", "handleMoreMenu", function(component, menuItemIndex)
{
    menuSelections.push(menuItemIndex);
});
```
```json:testMetadata:left-click-context-menu
{
  "testable": false,
  "skipReason": "Context menu requires physical user interaction (left-click) that cannot be triggered programmatically from script"
}
```

**Pitfalls:**
- The state function results are cached and only refreshed after a menu selection or an explicit `refreshContextMenuState()` call. If external state changes between menu openings, call `refreshContextMenuState()` to update the cached tick marks and enabled states.
