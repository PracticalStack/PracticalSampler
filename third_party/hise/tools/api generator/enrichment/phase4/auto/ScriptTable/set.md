Sets a component property to the given value. During `onInit`, changes are applied without UI notification; outside `onInit`, the UI updates automatically.

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `text`, `tooltip` | Display text and hover tooltip |
| `bgColour`, `itemColour`, `itemColour2`, *`customColours`* | Colour properties and custom colour rendering |
| `parentComponent` | Parent component for layout nesting |
| `saveInPreset`, `useUndoManager`, `deferControlCallback` | Preset persistence, undo, and callback deferral |
| `processorId`, *`tableIndex`* | Complex data source: the connected processor and the table slot to use from that processor or external data holder |
