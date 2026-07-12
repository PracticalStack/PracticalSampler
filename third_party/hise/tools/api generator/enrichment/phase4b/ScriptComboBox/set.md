ScriptComboBox::set(String propertyName, NotUndefined value) -> undefined

Thread safety: UNSAFE
Sets a component property to the given value. During onInit, changes apply without UI notification;
outside onInit, sends change notifications to update the UI.

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

Deactivated properties: `min` (fixed at 1), `max` (auto-managed from Items count).
Dispatch/mechanics:
  setScriptObjectPropertyWithChangeMessage(id, value)
    -> if property is "items": updates Items property, recalculates max from item count
    -> delegates to ScriptComponent base for all other properties
Pair with:
  get -- read back property values
  getAllProperties -- list available property IDs
Anti-patterns:
  - Do NOT set "items" with comma-separated text -- items must be newline-separated.
    Commas create a single item containing the full text.
Source:
  ScriptingApiContent.cpp:2988  ScriptComboBox::setScriptObjectPropertyWithChangeMessage()
    -> updates max to getItemList().size() when Items property changes
