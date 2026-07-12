Sets a component property to the given value. During `onInit`, changes are applied without UI notification; outside `onInit`, the UI updates automatically.

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `text`, `tooltip` | Display text (shown when nothing is selected) and hover tooltip |
| *`items`* | Newline-separated list of selectable items; setting this auto-updates the value range |
| *`fontName`*, *`fontSize`*, *`fontStyle`* | Font family, size, and style for the displayed text |
| `bgColour`, `itemColour`, `itemColour2`, `textColour` | Colour properties |
| `parentComponent` | Parent component for layout nesting |
| `defaultValue` | Default value (1-based item index); `min`/`max` are auto-managed and not settable |
| `saveInPreset`, `useUndoManager`, `deferControlCallback` | Preset persistence, undo, and callback deferral |
| `isPluginParameter`, `pluginParameterName`, `pluginParameterGroup`, `automationID`, *`enableMidiLearn`* | DAW automation and MIDI learn |
| `macroControl`, `isMetaParameter`, `linkedTo` | Macro control and parameter linking |
| `processorId`, `parameterId` | Module parameter connection |
| *`popupAlignment`*, *`useCustomPopup`* | Popup menu position (`bottom`, `top`, `topRight`, `bottomRight`) and custom submenu/header/separator parsing |
