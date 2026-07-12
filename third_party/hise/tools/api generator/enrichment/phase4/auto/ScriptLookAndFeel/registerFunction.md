Registers a custom paint function that overrides the default rendering for a specific UI component type. Pass one of the predefined function names and a callback. Draw functions receive `(g, obj)` where `g` is a Graphics context and `obj` contains component-specific properties (area, colours, interaction state). Five data-returning functions receive only `(obj)` and must return a value: `getIdealPopupMenuItemSize`, `getThumbnailRenderOptions`, `getAlertWindowMarkdownStyleData`, `createPresetBrowserIcons`, and `getModulatorDragData`.

The `obj` properties differ per function name - `drawRotarySlider` provides `valueNormalized`, `modulationActive`, and scaling data, while `drawToggleButton` provides `value`, `over`, `down`, and `text`. Consult the LAF function reference for the exact properties available for each function name.

When registering the same draw function on multiple LAF objects (e.g. popup menu rendering shared across several LAFs), define it once as a named `inline function` and pass it by reference rather than duplicating anonymous functions.

> [!Warning:Error in any function halts all rendering] If any registered paint function throws a script error, all subsequent paint calls are silently skipped until the script is recompiled. A single broken function disables all custom rendering across the entire LAF.
