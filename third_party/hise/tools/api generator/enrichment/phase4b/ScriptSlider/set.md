ScriptSlider::set(String propertyName, NotUndefined value) -> undefined

Thread safety: UNSAFE
Sets a component property to the given value. Reports a script error if the
property does not exist. During onInit, changes are applied without UI notification;
outside onInit, sends change notifications to update the UI.

Properties (italic = component-specific):

| Property | Description |
|----------|-------------|
| x, y, width, height | Position and size in pixels, relative to parent |
| visible, enabled, locked | Display and interaction state |
| text, tooltip | Display text and hover tooltip |
| min, max, defaultValue, *mode*, *middlePosition*, *stepSize*, *suffix* | Value range, mode, stepping, and display suffix |
| bgColour, itemColour, itemColour2, textColour | Colour properties |
| parentComponent | Parent component for layout nesting |
| saveInPreset, useUndoManager, deferControlCallback, *sendValueOnDrag* | Preset persistence, undo, callback deferral, and drag value updates |
| isPluginParameter, pluginParameterName, pluginParameterGroup, automationID, *enableMidiLearn* | DAW automation and MIDI learn |
| macroControl, isMetaParameter, linkedTo | Macro control and parameter linking |
| processorId, parameterId | Module parameter connection |
| *style* | Visual style: Knob, Horizontal, Vertical, Range |
| *filmstripImage*, *numStrips*, *isVertical*, *scaleFactor* | Filmstrip rendering |
| *showValuePopup*, *showTextBox* | Value display feedback |
| *mouseSensitivity*, *dragDirection*, *scrollWheel* | Interaction behavior |

Anti-patterns:
  - Do NOT use an invalid property name -- throws a script error

Pair with:
  get -- read back a property value
  setPropertiesFromJSON -- batch property assignment

Source:
  ScriptingApiContent.cpp  ScriptComponent::set()
    -> property tree mutation with optional change notification
