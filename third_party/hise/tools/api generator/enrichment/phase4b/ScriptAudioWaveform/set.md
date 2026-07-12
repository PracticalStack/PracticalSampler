ScriptAudioWaveform::set(String propertyName, NotUndefined value) -> undefined

Thread safety: UNSAFE
Sets a component property to the given value. Reports a script error if the
property does not exist. During onInit, changes are applied without UI notification;
outside onInit, sends change notifications to update the UI.

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

Deactivated properties: `macroControl`, `parameterId`, `linkedTo`, `isMetaParameter`, `isPluginParameter`, `pluginParameterName`, `automationID`, `text`, `min`, `max`, `defaultValue`.

Pair with:
  get -- to read the property value
  getAllProperties -- to discover valid property names

Source:
  ScriptingApiContent.cpp  ScriptComponent::set()
    -> setScriptObjectPropertyWithChangeMessage()
