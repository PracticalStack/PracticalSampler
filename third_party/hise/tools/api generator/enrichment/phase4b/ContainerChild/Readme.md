ContainerChild (object)
Obtain via: ScriptDynamicContainer.setData(jsonData)

Reference handle to a child component inside a ScriptDynamicContainer. Provides
property get/set with dot-assignment syntax, recursive hierarchy traversal,
custom paint routines, value and child-change callbacks, dynamic child
manipulation (add/remove), Base64 serialization, and user preset persistence.

Common mistakes:
  - Storing a ContainerChild reference and using it after calling setData()
    again on the parent -- setData() invalidates ALL previous references,
    causing script errors on use.
  - Expecting setValue() to trigger the control callback -- setValue() writes
    silently. Call changed() afterward to fire the callback and visual refresh.
  - Reading a property via dot syntax and expecting a default value -- dot-read
    (cc.text) returns raw ValueTree property (possibly undefined). Use
    cc.get("propertyName") for default fallback.

Example:
  const var dc = Content.addDynamicContainer("MyContainer", 0, 0);
  dc.setPosition(0, 0, 500, 300);

  const var data = [
  {
      "id": "Knob1",
      "type": "Slider",
      "text": "Volume",
      "min": 0.0,
      "max": 1.0,
      "x": 10, "y": 10, "width": 128, "height": 48
  }];

  const var cc = dc.setData(data);

  // Access a child by ID
  const var knob = cc.getComponent("Knob1");

  knob.setControlCallback(function(value)
  {
      Console.print("Knob value: " + value);
  });

Methods (28):
  addChildComponent          addStateToUserPreset       changed
  fromBase64                 get                        getAllComponents
  getChildComponentIndex     getComponent               getLocalBounds
  getNumChildComponents      getParent                  getValue
  isEqual                    isValid                    loseFocus
  removeAllChildren          removeFromParent           resetValueToDefault
  sendRepaintMessage         set                        setBounds
  setChildCallback           setControlCallback         setPaintRoutine
  setValue                   setValueWithUndo           toBase64
  updateValueFromProcessorConnection
