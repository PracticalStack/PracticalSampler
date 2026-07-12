Sets a component property to the given value. During `onInit`, changes are applied without UI notification; outside `onInit`, the UI updates automatically.

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `visible`, `enabled`, `locked`, *`opaque`* | Display and interaction state |
| `tooltip` | Hover tooltip (`text` is deactivated) |
| `bgColour`, `itemColour`, `itemColour2`, `textColour`, *`itemColour3`* | Colour properties for the waveform, overlays, and additional waveform accents |
| `parentComponent` | Parent component for layout nesting |
| `saveInPreset`, `useUndoManager`, `deferControlCallback` | Preset persistence, undo, and callback deferral |
| `processorId`, *`sampleIndex`* | Complex data source: the connected processor and the audio-file slot to use from that processor or external data holder |
| *`showLines`*, *`showFileName`*, *`enableRange`*, *`loadWithLeftClick`* | Waveform display and interaction behavior |
