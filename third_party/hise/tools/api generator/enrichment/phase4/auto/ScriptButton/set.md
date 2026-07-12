Sets a component property to the given value. During `onInit`, changes are applied without UI notification; outside `onInit`, the UI updates automatically.

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `visible`, `enabled`, `locked`, *`mouseCursor`* | Display and interaction state |
| `text`, `tooltip` | Display text and hover tooltip |
| `bgColour`, `itemColour`, `itemColour2`, `textColour` | Colour properties |
| `parentComponent` | Parent component for layout nesting |
| `saveInPreset`, `useUndoManager`, `deferControlCallback`, *`setValueOnClick`* | Preset persistence, undo, callback deferral, and value behavior |
| `isPluginParameter`, `pluginParameterName`, `pluginParameterGroup`, `automationID`, *`enableMidiLearn`* | DAW automation and MIDI learn |
| `macroControl`, `isMetaParameter`, `linkedTo` | Macro control and parameter linking |
| `processorId`, `parameterId` | Module parameter connection |
| *`filmstripImage`*, *`numStrips`*, *`isVertical`*, *`scaleFactor`* | Filmstrip rendering |
| *`isMomentary`*, *`radioGroup`* | Button behavior (momentary mode, radio group exclusivity) |
