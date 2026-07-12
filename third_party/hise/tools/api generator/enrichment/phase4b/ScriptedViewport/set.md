ScriptedViewport::set(String propertyName, NotUndefined value) -> undefined

Thread safety: UNSAFE
Sets a component property to the given value. Reports a script error if the
property does not exist. During onInit, changes are applied without UI notification;
outside onInit, sends change notifications to update the UI.

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `text`, `tooltip` | Display text and hover tooltip |
| `bgColour`, `itemColour`, `itemColour2`, `textColour` | Colour properties |
| `parentComponent` | Parent component for layout nesting |
| `saveInPreset`, `useUndoManager`, `deferControlCallback` | Preset persistence, undo, and callback deferral |
| `isPluginParameter`, `pluginParameterName`, `pluginParameterGroup`, `automationID`, `isMetaParameter`, `linkedTo` | Automation and parameter-linking support |
| `processorId`, `parameterId` | Standard parameter connection |
| `defaultValue`, *`items`* | Default selection value and newline-separated item list |
| *`fontName`*, *`fontSize`*, *`fontStyle`*, *`alignment`* | Font family, size, style, and text alignment |
| *`scrollBarThickness`*, *`autoHide`* | Scrollbar size and auto-hide behavior |
| *`viewPositionX`*, *`viewPositionY`* | Current scroll position |
| *`useList`* | Enables list mode instead of plain viewport mode |

Deactivated properties: `macroControl`, `min`, `max`.

Dispatch/mechanics: Validates property exists. Sets value on property tree. Outside onInit, sends change notification via sendChangeMessage(). viewPositionX/Y changes broadcast via positionBroadcaster.
Pair with: get (reads property values), getAllProperties (lists available properties)
Source:
  ScriptingApiContent.cpp  ScriptComponent::set() -> setScriptObjectProperty() -> sendChangeMessage()
