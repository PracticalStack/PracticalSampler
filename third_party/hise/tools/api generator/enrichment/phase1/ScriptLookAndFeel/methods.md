# ScriptLookAndFeel -- Method Analysis

## isImageLoaded

**Signature:** `Integer isImageLoaded(String prettyName)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String parameter involvement, atomic ref-count operations on var-to-String conversion.
**Minimal Example:** `var loaded = {obj}.isImageLoaded("background");`

**Description:**
Returns true if an image with the given alias has been loaded via `loadImage()`. Performs a linear search through the loaded images list comparing prettyName strings.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| prettyName | String | no | The alias assigned when the image was loaded via `loadImage()` | Case-sensitive match |

**Cross References:**
- `$API.ScriptLookAndFeel.loadImage$`
- `$API.ScriptLookAndFeel.unloadAllImages$`

## loadImage

**Signature:** `undefined loadImage(String imageName, String prettyName)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.loadImage("{PROJECT_FOLDER}myBg.png", "background");`

**Description:**
Loads an image from the project's Images folder (or an expansion pack) and stores it under the given alias. The alias is used to reference the image in paint functions via the Graphics API's `drawImage()`. If an image with the same prettyName already exists and the file reference differs, the image is silently replaced. If the image file is not found, a console warning is printed but no script error is thrown. Uses `TimeoutExtender` internally to prevent script timeout during disk I/O.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| imageName | String | no | Path to the image file relative to the Images folder | Supports expansion pack references via `{EXP::Name}` prefix |
| prettyName | String | no | Alias used to reference the image in paint functions | Must be unique per LAF instance; reusing an alias replaces the image |

**Pitfalls:**
- If the image file is not found, only a console warning is printed. No script error is thrown and no image is stored. Use `isImageLoaded()` after loading to verify the image was found.

**Cross References:**
- `$API.ScriptLookAndFeel.isImageLoaded$`
- `$API.ScriptLookAndFeel.unloadAllImages$`

## registerFunction

**Signature:** `undefined registerFunction(String functionName, Function paintFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.registerFunction("drawToggleButton", paintButton);`

**Description:**
Registers a custom paint function that overrides the default rendering for a specific UI component type. When HISE renders a component, it checks whether a function is registered for that draw operation and calls it instead of the default JUCE LookAndFeel rendering. If no function is registered for a given operation, the default rendering is used. This allows selective overriding -- register only the component types you want to customize.

Draw functions receive `(g, obj)` where `g` is a Graphics context and `obj` is a plain object with component-specific properties (area, colours, interaction state). Five data-returning functions receive only `(obj)` and must return a value: `getIdealPopupMenuItemSize`, `getThumbnailRenderOptions`, `getAlertWindowMarkdownStyleData`, `createPresetBrowserIcons`, `getModulatorDragData`.

If any registered paint function throws a script error during rendering, all subsequent paint calls are halted until the script is recompiled. This prevents cascading errors from flooding the console.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| functionName | String | no | Name of the draw operation to override | Must be one of the 62 predefined function names (see Value Descriptions) |
| paintFunction | Function | no | The callback to invoke during rendering | Draw functions: `function(g, obj)`. Data functions: `function(obj)` returning a value |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "drawAlertWindow" | Draws the background and layout of system alert/dialog windows |
| "getAlertWindowMarkdownStyleData" | Returns style data object for alert window markdown rendering (data function) |
| "drawAlertWindowIcon" | Draws the icon in alert/dialog windows |
| "drawPopupMenuBackground" | Draws the background of popup/context menus |
| "drawPopupMenuItem" | Draws individual items in popup/context menus |
| "getIdealPopupMenuItemSize" | Returns the ideal size for popup menu items (data function) |
| "drawToggleButton" | Draws toggle/script buttons |
| "drawRotarySlider" | Draws rotary-style sliders (knobs) |
| "drawLinearSlider" | Draws linear-style sliders (horizontal/vertical) |
| "drawDialogButton" | Draws buttons inside dialog windows |
| "drawComboBox" | Draws combo box components |
| "drawNumberTag" | Draws macro control number tag overlays |
| "createPresetBrowserIcons" | Returns path data for preset browser navigation icons (data function) |
| "drawPresetBrowserBackground" | Draws the preset browser main background |
| "drawPresetBrowserDialog" | Draws preset browser dialog overlays |
| "drawPresetBrowserColumnBackground" | Draws preset browser column backgrounds |
| "drawPresetBrowserListItem" | Draws individual items in preset browser lists |
| "drawPresetBrowserSearchBar" | Draws the preset browser search bar |
| "drawPresetBrowserTag" | Draws preset browser tag labels |
| "drawWavetableBackground" | Draws the background of wavetable displays |
| "drawWavetablePath" | Draws the waveform path in wavetable displays |
| "drawTableBackground" | Draws the background of table curve editors |
| "drawTablePath" | Draws the curve path in table editors |
| "drawTablePoint" | Draws control points on table curves |
| "drawTableMidPoint" | Draws mid-points (tension handles) on table curves |
| "drawTableRuler" | Draws the ruler/position indicator on table editors |
| "drawScrollbar" | Draws scrollbars in any scrollable component |
| "drawMidiDropper" | Draws the MIDI file dropper component |
| "drawThumbnailBackground" | Draws the background of audio waveform thumbnails |
| "drawThumbnailText" | Draws text overlays on audio thumbnails |
| "drawThumbnailPath" | Draws the waveform path in audio thumbnails |
| "drawThumbnailRange" | Draws the selected range on audio thumbnails |
| "drawThumbnailRuler" | Draws the playback position ruler on audio thumbnails |
| "getThumbnailRenderOptions" | Returns rendering configuration for audio thumbnail display (data function) |
| "drawAhdsrBackground" | Draws the background of AHDSR envelope displays |
| "drawAhdsrBall" | Draws the position ball on AHDSR envelope displays |
| "drawAhdsrPath" | Draws the envelope curve path on AHDSR displays |
| "drawKeyboardBackground" | Draws the background of virtual keyboard components |
| "drawWhiteNote" | Draws white keys on virtual keyboards |
| "drawBlackNote" | Draws black keys on virtual keyboards |
| "drawSliderPackBackground" | Draws the background of slider pack components |
| "drawSliderPackFlashOverlay" | Draws the flash overlay effect on active slider pack sliders |
| "drawSliderPackRightClickLine" | Draws the line indicator during right-click drag on slider packs |
| "drawSliderPackTextPopup" | Draws the value text popup on slider pack hover |
| "drawTableRowBackground" | Draws row backgrounds in table list views (viewport table mode) |
| "drawTableCell" | Draws individual cells in table list views |
| "drawTableHeaderBackground" | Draws the header background in table list views |
| "drawTableHeaderColumn" | Draws individual header columns in table list views |
| "drawFilterDragHandle" | Draws the drag handle for draggable filter panels |
| "drawFilterBackground" | Draws the background of filter display components |
| "drawFilterPath" | Draws the filter curve path on filter displays |
| "drawFilterGridLines" | Draws grid lines on filter displays |
| "drawAnalyserBackground" | Draws the background of audio analyser components |
| "drawAnalyserPath" | Draws the analysis path (spectrum, waveform) on analyser displays |
| "drawAnalyserGrid" | Draws grid lines on audio analyser displays |
| "drawMatrixPeakMeter" | Draws the matrix peak meter component |
| "getModulatorDragData" | Returns data for modulation drag indicator display (data function) |
| "drawModulationDragBackground" | Draws the background of modulation drag indicators |
| "drawModulationDragger" | Draws the modulation drag handle |
| "drawFlexAhdsrBackground" | Draws the background of flex AHDSR envelope displays |
| "drawFlexAhdsrCurvePoint" | Draws curve control points on flex AHDSR displays |
| "drawFlexAhdsrFullPath" | Draws the full envelope path on flex AHDSR displays |
| "drawFlexAhdsrPosition" | Draws the playback position indicator on flex AHDSR displays |
| "drawFlexAhdsrSegment" | Draws individual segments of flex AHDSR envelopes |
| "drawFlexAhdsrText" | Draws text labels on flex AHDSR displays |

**Callback Signature:** paintFunction(g: Graphics, obj: Object) for draw functions; paintFunction(obj: Object) for data-returning functions

**Pitfalls:**
- [BUG] Invalid function names are silently accepted. The function is stored internally but never invoked because the rendering code only looks up the 62 predefined names. No error or warning is produced -- the component uses default rendering and the user has no indication that the name was wrong.
- [BUG] If the second argument is not a valid JavaScript function, the call silently does nothing. No error is thrown.

**Cross References:**
- `$API.ScriptLookAndFeel.setInlineStyleSheet$`
- `$API.ScriptLookAndFeel.setStyleSheet$`
- `$API.ScriptLookAndFeel.setGlobalFont$`

**Example:**

```javascript:custom-toggle-button-laf
// Title: Custom toggle button with circle indicator
const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawToggleButton", function(g, obj)
{
    var a = obj.area;
    g.setColour(obj.bgColour);
    g.fillRoundedRectangle(a, 3.0);

    if (obj.value)
    {
        g.setColour(obj.itemColour1);
        g.fillEllipse([a[0] + 4, a[1] + 4, a[2] - 8, a[3] - 8]);
    }
});

const var btn = Content.addButton("LafButton", 10, 10);
btn.setLocalLookAndFeel(laf);
```

```json:testMetadata:custom-toggle-button-laf
{
  "testable": false,
  "skipReason": "Visual rendering output cannot be verified programmatically"
}
```

## setGlobalFont

**Signature:** `undefined setGlobalFont(String fontName, Double fontSize)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setGlobalFont("Oxygen Bold", 16.0);`

**Description:**
Sets the font used by all JUCE LookAndFeel font getter methods, including alert window fonts, popup menu fonts, combo box fonts, text button fonts, and dialog fonts. The default is `GLOBAL_BOLD_FONT()`. This does NOT affect fonts used inside registered paint functions -- those are controlled by `g.setFont()` within each callback. The font is resolved via `MainController::getFontFromString()`, which supports fonts loaded with `Engine.loadFontAs()` and system fonts.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| fontName | String | no | Name of the font to use | Must be a font loaded via `Engine.loadFontAs()` or a system font name |
| fontSize | Double | no | Font size in pixels | Positive value |

**Pitfalls:**
- Only affects fonts rendered by JUCE's built-in LookAndFeel methods (alert windows, popup menus, combo boxes, text buttons, dialog buttons). Fonts inside registered paint functions are unaffected and must be set explicitly via `g.setFont()` in each callback.

**Cross References:**
- `$API.Engine.loadFontAs$`
- `$API.ScriptLookAndFeel.registerFunction$`

## setInlineStyleSheet

**Signature:** `undefined setInlineStyleSheet(String cssCode)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setInlineStyleSheet("button { background: #333; }");`

**Description:**
Parses and applies CSS code provided directly as a string, activating CSS rendering mode for this LAF. A unique file identifier is generated from the hash of the CSS code for internal tracking. If the CSS contains syntax errors, a script error is thrown immediately. Passing an empty string disables CSS mode. Acquires a write lock on the `LookAndFeelRenderLock` during application, which blocks any concurrent paint calls. When both CSS and registered script functions are active on a local LAF, a combined rendering mode is used where each draw operation checks for a registered script function first, falling back to CSS.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| cssCode | String | no | CSS stylesheet code to parse and apply | Must be valid CSS syntax; empty string disables CSS mode |

**Cross References:**
- `$API.ScriptLookAndFeel.setStyleSheet$`
- `$API.ScriptLookAndFeel.setStyleSheetProperty$`
- `$API.ScriptLookAndFeel.registerFunction$`

**Example:**

```javascript:inline-css-styling
// Title: Inline CSS styling for buttons with hover state
const var laf = Content.createLocalLookAndFeel();

laf.setInlineStyleSheet("button { background: #333333; color: white; border-radius: 5px; } button:hover { background: #555555; }");

const var btn = Content.addButton("CssButton", 10, 10);
btn.setLocalLookAndFeel(laf);
```

```json:testMetadata:inline-css-styling
{
  "testable": false,
  "skipReason": "Visual rendering output cannot be verified programmatically"
}
```

## setStyleSheet

**Signature:** `undefined setStyleSheet(String fileName)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setStyleSheet("interface.css");`

**Description:**
Loads and applies a CSS stylesheet from a file in the project's Scripts folder. The file must have the `.css` extension or a script error is thrown. The file content is loaded and parsed via the same internal path as `setInlineStyleSheet()`. In the HISE IDE (`USE_BACKEND`), the file is watched for live editing -- changes are reflected immediately without recompilation. If the file does not exist in the IDE, a default template (`* { color: white; }`) is created automatically. In exported plugins, the CSS file is loaded from the embedded script collection.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| fileName | String | no | Path to the CSS file relative to the Scripts folder | Must have `.css` extension |

**Pitfalls:**
- In the HISE IDE, a missing CSS file is created automatically with a minimal default template. This means a typo in the filename silently creates a new file instead of producing an error. The auto-creation only happens in the IDE -- in exported plugins, a missing file returns an empty string from the collection.

**Cross References:**
- `$API.ScriptLookAndFeel.setInlineStyleSheet$`
- `$API.ScriptLookAndFeel.setStyleSheetProperty$`
- `$API.ScriptLookAndFeel.registerFunction$`

## setStyleSheetProperty

**Signature:** `undefined setStyleSheetProperty(String variableId, NotUndefined value, String type)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setStyleSheetProperty("bgColor", 0xFF333333, "color");`

**Description:**
Sets a CSS variable on the LAF's `additionalProperties` ValueTree. The value is converted to a CSS-compatible string according to the specified type before storage. These properties are accessible in CSS stylesheets via `var(--variableId)` syntax. This enables dynamic theming and data binding between HISEScript and CSS -- script logic can update visual properties without rewriting the stylesheet.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| variableId | String | no | The CSS variable name (without the `--` prefix) | Valid CSS identifier |
| value | NotUndefined | no | The value to store | Type depends on the `type` parameter |
| type | String | no | Unit/format conversion type | One of: `""`, `"px"`, `"%"`, `"em"`, `"vh"`, `"deg"`, `"color"`, `"path"` |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "" | No conversion; value is stored as-is |
| "px" | Appends "px" unit suffix to the numeric value |
| "%" | Multiplies numeric value by 100 and appends "%" suffix |
| "em" | Appends "em" unit suffix to the numeric value |
| "vh" | Appends "vh" unit suffix to the numeric value |
| "deg" | Appends "deg" unit suffix to the numeric value |
| "color" | Converts integer colour (0xAARRGGBB) to CSS colour string "#AARRGGBB" |
| "path" | Converts a Path object to a base64-encoded string for CSS `background-image` |

**Cross References:**
- `$API.ScriptLookAndFeel.setInlineStyleSheet$`
- `$API.ScriptLookAndFeel.setStyleSheet$`

**Example:**

```javascript:dynamic-css-theming
// Title: Dynamic CSS theming with style sheet properties
const var laf = Content.createLocalLookAndFeel();

laf.setInlineStyleSheet("button { background: var(--bgColor); border-radius: var(--radius); }");

laf.setStyleSheetProperty("bgColor", 0xFF444444, "color");
laf.setStyleSheetProperty("radius", 5.0, "px");

const var btn = Content.addButton("ThemedBtn", 10, 10);
btn.setLocalLookAndFeel(laf);
```

```json:testMetadata:dynamic-css-theming
{
  "testable": false,
  "skipReason": "Visual rendering output cannot be verified programmatically"
}
```

## unloadAllImages

**Signature:** `undefined unloadAllImages()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.unloadAllImages();`

**Description:**
Removes all images previously loaded via `loadImage()`, releasing their pooled references. After calling this, `isImageLoaded()` returns false for all previously loaded aliases and paint functions can no longer reference the unloaded images.

**Cross References:**
- `$API.ScriptLookAndFeel.loadImage$`
- `$API.ScriptLookAndFeel.isImageLoaded$`
