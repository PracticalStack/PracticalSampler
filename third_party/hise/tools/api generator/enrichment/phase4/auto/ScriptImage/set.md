Sets a component property to the given value. During `onInit`, changes are applied without UI notification; outside `onInit`, the UI updates automatically.

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `tooltip` | Hover tooltip (`text` is deactivated) |
| *`alpha`*, *`offset`*, *`scale`* | Image opacity, offset, and scaling |
| *`fileName`*, *`blendMode`* | Source image file and blend mode |
| *`allowCallbacks`* | Mouse callback level for image interaction |
| *`popupMenuItems`*, *`popupOnRightClick`* | Popup menu content and trigger mode |
| `parentComponent` | Parent component for layout nesting |
| `saveInPreset`, `useUndoManager`, `deferControlCallback` | Preset persistence, undo, and callback deferral |
| `isPluginParameter`, `pluginParameterName`, `pluginParameterGroup`, `isMetaParameter` | Automation and meta-parameter support (`automationID` is deactivated) |
| `processorId`, `parameterId` | Module parameter connection |
