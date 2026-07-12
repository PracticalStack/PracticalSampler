# ScriptPanel — UI Component Page Content

**Purpose:** Content extracted from the old `scriptpanel.md` reference guide that belongs on the **UI component reference page** (properties, visual styling, component-level behaviour). The scripting API methods, callback systems, and programming patterns are in `scriptpanel_api.md`.

**Source:** `ui_enrichment/resources/scriptpanel.md` (old docs)

---

## Overview (for UI component page)

The ScriptPanel is the most versatile UI component in HISE. It starts as a simple background panel for grouping other UI elements, but can be customised into virtually any UI widget through three callback systems (paint routine, mouse events, timer) and CSS styling.

Unlike other components that have a fixed visual appearance with LAF customisation, the ScriptPanel is a blank canvas — its appearance is entirely defined by the developer through either:
1. A **paint routine** (programmatic drawing via the Graphics API)
2. **CSS styling** (declarative, using the `div` / `.scriptpanel` selectors)
3. A **fixed image** (via `setImage()`)

---

## Properties specific to ScriptPanel

These properties are unique to the ScriptPanel and control its behaviour as a UI element. The full property table comes from phase4b `set.md`, but these deserve expanded descriptions on the UI page:

### `allowCallbacks`

Controls what mouse events the panel responds to. This is the most important property for interactive panels — it gates all mouse callback functionality.

| Value | Events fired |
|-------|-------------|
| `"No Callbacks"` | Nothing (default) |
| `"Context Menu"` | Nothing directly — shows a popup menu instead |
| `"Clicks Only"` | Mouse clicks and releases |
| `"Clicks & Hover"` | Clicks + entering/leaving the panel |
| `"Clicks, Hover & Dragging"` | Clicks + hover + mouse drag with button down |
| `"All Callbacks"` | All of the above + continuous mouse movement |

**Best practice:** Use the minimum level needed. `"Clicks & Hover"` is sufficient for most interactive widgets. `"All Callbacks"` fires on every mouse move and should be avoided unless continuous tracking is truly required.

### `opaque`

When `true`, tells the renderer that this panel fills its entire area with solid content and its parent does not need repainting when the panel changes. Set this for any non-transparent panel to improve rendering performance.

**Default:** `false`

### `bufferToImage`

When `true`, the paint routine output is cached as a bitmap. Subsequent repaints only update the cached image. Useful for complex paint routines that don't change frequently.

**Default:** `false`

### `allowDragging`

Enables the panel's built-in drag behaviour for internal drag-and-drop operations (used with `startInternalDrag()`).

### Popup menu properties

These properties configure the built-in context menu system:

| Property | Description |
|----------|-------------|
| `PopupMenuItems` | Newline-separated list of menu items. Supports advanced syntax (see below). |
| `PopupOnRightClick` | If `true`, right-clicking shows the popup menu |
| `popupMenuAlign` | If `true`, aligns the popup to the panel's width and bottom edge |
| `selectedPopupIndex` | The currently selected popup item index |
| `holdIsRightClick` | If `true`, a long press (touch) triggers a right-click/popup |
| `isPopupPanel` | Enables popup-panel mode |

### Popup menu syntax

The `PopupMenuItems` property supports a rich syntax for creating structured context menus:

```
Item 1                        | Normal item (clickable)
**Header**                    | Section title (not clickable)
___                           | Separator (three underscores, not clickable)
~~Deactivated Item~~          | Disabled item (not clickable)
MySubMenu::First SubItem      | Item in submenu (clickable)
MySubMenu::**Sub Header**     | Header in submenu (not clickable)
MySubMenu::~~Second SubItem~~ | Disabled item in submenu (not clickable)
```

> This popup menu syntax is also available in `Broadcaster.attachToContextMenu()` and in ComboBox with `useCustomPopup` enabled.

The mouse event callback receives `event.result` (one-based index of clickable items only, 0 if dismissed) and `event.itemText` (the selected item's text).

### `borderSize` and `borderRadius`

Panel-specific styling properties for the default appearance (without a paint routine or CSS). These control the border drawn around the panel in its default rendering mode.

### Value range properties

The panel supports `min`, `max`, `defaultValue`, and `stepSize` for its stored value. This is relevant when using the panel as a custom control that stores numeric state via `setValue()` / `getValue()`.

---

## CSS Styling

The ScriptPanel supports CSS styling as an alternative to paint routines. See `css_component_mapping.md` for the full selector reference.

**Key points for the UI page:**
- HTML tag selector: `div`
- Class selector: `.scriptpanel`
- ID selector: `#Panel1`
- Supports `:hover` and `:active` pseudo-states (requires `allowCallbacks` to be set)
- Supports `::before` and `::after` pseudo-elements
- Custom CSS variables can be passed via `setStyleSheetProperty()`
- The `content` property can render text from CSS variables: `content: var(--titleText)`
- Transitions work for smooth state changes

**CSS example** (from old docs):

```javascript
const var p = Content.addPanel("Panel1", 10, 10);

p.set("allowCallbacks", "Clicks & Hover");

const var laf = Content.createLocalLookAndFeel();

laf.setInlineStyleSheet("
div
{
	content: var(--titleText);
	vertical-align: top;
	padding: 5px;
	color: white;
	margin: 10px;
	box-shadow: 0px 0px 5px yellow;
	font-weight: bold;
	border-radius: 3px;
	background: red;
}

div:hover
{
	background: blue;
}

div:active
{
	background: black;
	margin: 10px;
	transition: all 0.5s ease-out;
	box-shadow: none;
}

");

p.setLocalLookAndFeel(laf);
p.setStyleSheetProperty("titleText", "Title", "");
```

---

## What does NOT go on the UI page

The following content from `scriptpanel.md` belongs in the **scripting API docs** (see `scriptpanel_api.md`):

- Paint routine tutorial (Graphics API usage, colours, areas, images, fonts, paths)
- Mouse event callback tutorial (event properties table, callback patterns)
- Timer callback tutorial
- Data storage patterns (`this.data` vs `setValue`/`getValue`)
- Performance best practices (repaint rate, `repaintImmediately()`)
- Factory method patterns and namespace encapsulation
- Worked examples (six-state button, ButtonPack, rotatable head, vector knob)
