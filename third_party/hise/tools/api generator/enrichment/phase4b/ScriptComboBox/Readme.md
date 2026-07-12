ScriptComboBox (object)
Obtain via: Content.addComboBox(name, x, y)

Drop-down list component for selecting from named items using 1-based integer
indexing. Items are stored as a newline-separated string. Supports dynamic item
management, custom popup menus with submenus, headers, separators, and disabled
items, plus host automation via plugin parameters.

When useCustomPopup is enabled, the item string supports these special forms:
  - **HeaderText** for non-selectable section headers
  - ___ for separator lines
  - Category::ItemName for submenu items
  - ~~DisabledItem~~ for greyed-out disabled entries

Headers and separators do not consume selection indices, so the selected value
counts only real selectable items.

Complexity tiers:
  1. Static selector: set("items", ...), setControlCallback, getValue. Fixed-option
     selectors for oscillator type, filter mode, etc.
  2. Dynamic file selector: addItem in a loop after FileSystem.findFiles(), getItemText
     in callback to resolve selection. Set saveInPreset to false.
  3. Cascading dependent lists: set("items", ...) to rebuild dependent list, setValue
     to clamp selection, changed() to trigger dependent callback chain.
  4. Fully styled selector: + setLocalLookAndFeel with drawComboBox, drawPopupMenuBackground,
     drawPopupMenuItem. Custom rendering for both closed box and popup.

Practical defaults:
  - Set saveInPreset to false for combo boxes with dynamically scanned file lists.
    Item lists differ between machines, so persisting the index is meaningless.
  - Clear items with set("items", "") before populating with addItem() to avoid
    stale entries from the Interface Designer.
  - Use parseInt(value) when using combo box value as an array index. Values arrive
    as floats in callbacks (e.g., 1.0 not 1).
  - Use value - 1 (after parseInt) to convert from 1-based combo box index to
    0-based array index.

Common mistakes:
  - Using value directly as array index -- combo box values are 1-based floats.
    Use array[parseInt(value) - 1] for 0-based access.
  - cb.setValue(0) to select first item -- value 0 means "nothing selected" and
    shows placeholder text. Use cb.setValue(1).
  - set("items", "A,B,C") -- items must be newline-separated, not comma-separated.
    Comma text appears as a single item.
  - Populating with addItem() without clearing first -- stale items from the
    Interface Designer remain. Clear with set("items", "") before rebuilding.
  - Saving dynamic file lists in presets -- file-scanned lists differ between
    machines. Persist file reference or processor state instead.
  - Styling only drawComboBox -- the popup menu uses separate draw functions
    (drawPopupMenuBackground, drawPopupMenuItem). Without them the popup renders
    with default styling.
  - Rebuilding dependent combo items without changed() -- the dependent callback
    chain does not fire, leaving UI in stale state.

Example:
  const var cb = Content.addComboBox("MyComboBox", 0, 0);
  cb.set("items", "Option A\nOption B\nOption C");

Methods (35):
  addItem                     addToMacroControl
  changed                     fadeComponent
  get                         getAllProperties
  getChildComponents          getGlobalPositionX
  getGlobalPositionY          getHeight
  getId                       getItemText
  getLocalBounds              getValue
  getValueNormalized          getWidth
  grabFocus                   loseFocus
  sendRepaintMessage          set
  setControlCallback          setConsumedKeyPresses
  setKeyPressCallback         setLocalLookAndFeel
  setPosition                 setStyleSheetClass
  setStyleSheetProperty       setStyleSheetPseudoState
  setTooltip                  setValue
  setValueNormalized          setValueWithUndo
  setZLevel                   showControl
  updateValueFromProcessorConnection
