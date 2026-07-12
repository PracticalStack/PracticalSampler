ScriptTable::set(String propertyName, NotUndefined value) -> undefined

Thread safety: UNSAFE
Sets a component property to the given value. Reports a script error if the
property does not exist. During onInit, changes are applied without UI notification;
outside onInit, sends change notifications to update the UI.

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `text`, `tooltip` | Display text and hover tooltip |
| `bgColour`, `itemColour`, `itemColour2`, *`customColours`* | Colour properties and custom colour rendering |
| `parentComponent` | Parent component for layout nesting |
| `saveInPreset`, `useUndoManager`, `deferControlCallback` | Preset persistence, undo, and callback deferral |
| `processorId`, *`tableIndex`* | Complex data source: the connected processor and the table slot to use from that processor or external data holder |

Deactivated properties: `min`, `max`, `defaultValue`, `textColour`, `parameterId`, `macroControl`, `linkedTo`, `isMetaParameter`, `isPluginParameter`, `pluginParameterName`, `automationID`.

Pair with:
  get -- read back one property
  getAllProperties -- inspect available properties

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:421  ScriptComponent API registration -> setScriptObjectPropertyWithChangeMessage()
