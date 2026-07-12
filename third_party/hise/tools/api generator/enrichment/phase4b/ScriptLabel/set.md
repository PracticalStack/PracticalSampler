ScriptLabel::set(String propertyName, NotUndefined value) -> undefined

Thread safety: UNSAFE
Sets a component property to the given value. During onInit, changes apply without UI notification;
outside onInit, sends change notifications to update the UI.

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

Deactivated properties: `defaultValue`, `min`, `max`, `automationID`.
The label's value is a string (the displayed text), not a number.
Dispatch/mechanics:
  setScriptObjectPropertyWithChangeMessage(id, value)
    -> if property is "text": calls setValue(newValue.toString()) to update displayed text
    -> delegates to ScriptComponent base for all other properties
Pair with:
  get -- read back property values
  getAllProperties -- list available property IDs
Source:
  ScriptingApiContent.cpp  ScriptLabel::setScriptObjectPropertyWithChangeMessage()
