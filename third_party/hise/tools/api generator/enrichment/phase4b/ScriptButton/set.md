ScriptButton::set(String propertyName, NotUndefined value) -> undefined

Thread safety: UNSAFE
Sets a component property to the given value. Reports a script error if the
property does not exist. During onInit, changes are applied without UI notification;
outside onInit, sends change notifications to update the UI.

Properties (italic = component-specific):

| Property | Description |
|----------|-------------|
| x, y, width, height | Position and size in pixels, relative to parent |
| visible, enabled, locked, *mouseCursor* | Display and interaction state |
| text, tooltip | Display text and hover tooltip |
| bgColour, itemColour, itemColour2, textColour | Colour properties |
| parentComponent | Parent component for layout nesting |
| saveInPreset, useUndoManager, deferControlCallback, *setValueOnClick* | Preset persistence, undo, callback deferral, and value behavior |
| isPluginParameter, pluginParameterName, pluginParameterGroup, automationID, *enableMidiLearn* | DAW automation and MIDI learn |
| macroControl, isMetaParameter, linkedTo | Macro control and parameter linking |
| processorId, parameterId | Module parameter connection |
| *filmstripImage*, *numStrips*, *isVertical*, *scaleFactor* | Filmstrip rendering |
| *isMomentary*, *radioGroup* | Button behavior (momentary mode, radio group exclusivity) |

Deactivated: min, max (button value is always 0 or 1)

Anti-patterns:
  - Do NOT use an invalid property name -- throws a script error

Pair with:
  get -- read back a property value
  setPropertiesFromJSON -- batch property assignment

Source:
  ScriptingApiContent.cpp  ScriptComponent::set()
    -> property tree mutation with optional change notification
