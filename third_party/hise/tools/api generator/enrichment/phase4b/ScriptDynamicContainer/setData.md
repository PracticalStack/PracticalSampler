ScriptDynamicContainer::setData(var newData) -> ScriptObject

Thread safety: UNSAFE -- creates dyncomp::Data objects with ValueTree allocation, invalidates previous ChildReferences, sends async UI rebuild notification via dataBroadcaster.
Creates a dynamic component tree from JSON data. Accepts a single JSON object (one
component) or an array of JSON objects (multiple). Returns a ContainerChild reference
-- to the single child if one object, or to the root tree if an array.
Supported types: "Button", "Slider", "ComboBox", "Label", "Panel", "FloatingTile",
"DragContainer", "Viewport", "TextBox", "TableEditor", "SliderPack", "AudioFile".
Legacy names (ScriptButton, ScriptSlider, etc.) auto-convert.
Required setup:
  const var dc = Content.addDynamicContainer("FXControls", 0, 0);
  dc.setPosition(0, 0, 400, 200);
Dispatch/mechanics:
  Invalidates all existing ChildReferences -> wraps single object in array
    -> creates dyncomp::Data from JSON -> dataBroadcaster.sendMessage(async)
    -> returns getOrCreateChildReference(dataTree)
Anti-patterns:
  - All previously returned ContainerChild references become invalid after calling
    setData(). Using an invalid reference throws a script error.
Pair with:
  setValueCallback -- register after setData to listen for child value changes
Source:
  ScriptingApiContent.cpp:6149  ScriptDynamicContainer::setData()
    -> invalidates childReferences -> new dyncomp::Data(mc, json, bounds)
    -> dataBroadcaster.sendMessage(sendNotificationAsync)
    -> getOrCreateChildReference(dt)
