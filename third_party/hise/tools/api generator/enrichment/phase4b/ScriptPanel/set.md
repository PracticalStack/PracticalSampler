ScriptPanel::set(String propertyName, NotUndefined value) -> undefined

Thread safety: UNSAFE -- modifies property tree, sends change notifications
Sets a component property to the given value. Reports a script error if the
property does not exist. During onInit, changes are applied without UI notification;
outside onInit, sends change notifications to update the UI.

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `visible`, `enabled`, `locked`, *`opaque`*, *`bufferToImage`* | Display state, opacity, and paint buffering |
| `text`, `tooltip` | Display text and hover tooltip |
| `min`, `max`, `defaultValue`, *`stepSize`* | Value range and stepping for the panel's stored value |
| `bgColour`, `itemColour`, `itemColour2`, `textColour`, *`borderSize`*, *`borderRadius`* | Colour and border styling |
| `parentComponent` | Parent component for layout nesting |
| `saveInPreset`, `useUndoManager`, `deferControlCallback` | Preset persistence, undo, and callback deferral |
| `pluginParameterName`, `pluginParameterGroup`, `automationID`, *`enableMidiLearn`* | Automation naming/grouping and MIDI learn (`isPluginParameter` defaults to false) |
| `macroControl`, `isMetaParameter` | Macro control assignment and meta-parameter behavior (`linkedTo` is deactivated) |
| `processorId`, `parameterId` | Module parameter connection |
| *`allowDragging`* | Enables dragging behavior for the panel |
| *`allowCallbacks`* | Mouse callback level for click, hover, and drag reporting |
| *`popupMenuItems`*, *`popupOnRightClick`*, *`popupMenuAlign`*, *`selectedPopupIndex`*, *`holdIsRightClick`*, *`isPopupPanel`* | Popup menu content, trigger mode, alignment, selection state, touch-hold behavior, and popup-panel mode |

Deactivated properties: `linkedTo`.
Pair with:
  get -- read property values
  getAllProperties -- list available property names
Source:
  ScriptingApiContent.cpp  ScriptComponent::set()
