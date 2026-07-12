Sets a component property to the given value. During `onInit`, changes are applied without UI notification; outside `onInit`, the UI updates automatically.

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `text`, `tooltip` | Display text and hover tooltip |
| `min`, `max`, `defaultValue`, *`mode`*, *`middlePosition`*, *`stepSize`*, *`suffix`* | Value range, mode, stepping, and display suffix |
| `bgColour`, `itemColour`, `itemColour2`, `textColour` | Colour properties |
| `parentComponent` | Parent component for layout nesting |
| `saveInPreset`, `useUndoManager`, `deferControlCallback`, *`sendValueOnDrag`* | Preset persistence, undo, callback deferral, and drag value updates |
| `isPluginParameter`, `pluginParameterName`, `pluginParameterGroup`, `automationID`, *`enableMidiLearn`* | DAW automation and MIDI learn |
| `macroControl`, `isMetaParameter`, `linkedTo` | Macro control and parameter linking |
| `processorId`, `parameterId` | Module parameter connection |
| *`style`* | Visual style: `Knob`, `Horizontal`, `Vertical`, `Range` |
| *`filmstripImage`*, *`numStrips`*, *`isVertical`*, *`scaleFactor`* | Filmstrip rendering |
| *`showValuePopup`*, *`showTextBox`* | Value display feedback |
| *`mouseSensitivity`*, *`dragDirection`*, *`scrollWheel`* | Interaction behavior |
