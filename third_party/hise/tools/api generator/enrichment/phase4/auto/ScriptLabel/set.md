Sets a component property to the given value. During `onInit`, changes are applied without UI notification; outside `onInit`, the UI updates automatically.

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `text`, `tooltip` | Display text (the label's value) and hover tooltip |
| *`fontName`*, *`fontSize`*, *`fontStyle`*, *`alignment`* | Font family, size, style, and text alignment |
| *`editable`*, *`multiline`*, *`updateEachKey`* | Text editing: whether the label is editable, supports multiple lines, and fires callbacks on each keystroke |
| `bgColour`, `itemColour`, `itemColour2`, `textColour` | Colour properties |
| `parentComponent` | Parent component for layout nesting |
| `saveInPreset`, `useUndoManager`, `deferControlCallback` | Preset persistence, undo, and callback deferral |
| `isPluginParameter`, `pluginParameterName`, `pluginParameterGroup` | DAW automation (`automationID` is deactivated) |
| `macroControl`, `isMetaParameter`, `linkedTo` | Macro control and parameter linking |
| `processorId`, `parameterId` | Module parameter connection |
