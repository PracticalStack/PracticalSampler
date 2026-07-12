ScriptDynamicContainer::set(String propertyName, NotUndefined value) -> undefined

Thread safety: UNSAFE
Sets a component property to the given value. Reports a script error if the
property does not exist. During onInit, changes apply without UI notification;
outside onInit, sends change notifications.

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `bgColour`, `itemColour`, `itemColour2`, `textColour` | Colour properties for the container and its wrapper |
| `parentComponent` | Parent component for layout nesting |
| `useUndoManager` | Undo support for container-level value changes |

Deactivated properties: `macroControl`, `isPluginParameter`, `min`, `max`, `defaultValue`, `pluginParameterName`, `text`, `tooltip`, `processorId`, `parameterId`, `isMetaParameter`, `linkedTo`, `automationID`, `deferControlCallback`, `pluginParameterGroup`, `saveInPreset`.

Pair with:
  get -- read a property value
  getAllProperties -- list available property names
Source:
  ScriptingApiContent.cpp  ScriptComponent::set()
    -> validates property -> sets on propertyTree -> sends change notification
