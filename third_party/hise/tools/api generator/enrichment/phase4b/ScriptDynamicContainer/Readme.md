ScriptDynamicContainer (object)
Obtain via: Content.addDynamicContainer(name, x, y)

Data-driven UI container that dynamically creates and manages child components
from JSON. Builds its component tree via setData() rather than individual
Content.addXXX() calls. Returns ContainerChild reference objects with dot-access
properties, hierarchy traversal, paint routines, value callbacks, and user preset
state serialization.

Common mistakes:
  - Calling setValueCallback() before setData() -- silently does nothing because
    the Values tree does not exist yet.
  - Storing ContainerChild references and using them after a new setData() call --
    all previous references are invalidated; throws script error.

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
  },
  {
      "id": "Btn1",
      "type": "Button",
      "text": "Bypass",
      "x": 150, "y": 10, "width": 128, "height": 32
  }];

  const var root = dc.setData(data);

  dc.setValueCallback(function(id, value)
  {
      Console.print(id + " = " + value);
  });

Methods (30):
  changed                 fadeComponent           get
  getAllProperties         getChildComponents      getGlobalPositionX
  getGlobalPositionY      getHeight               getId
  getLocalBounds          getValue                getWidth
  grabFocus               loseFocus               sendRepaintMessage
  set                     setConsumedKeyPresses   setControlCallback
  setData                 setKeyPressCallback     setLocalLookAndFeel
  setPosition             setStyleSheetClass      setStyleSheetProperty
  setStyleSheetPseudoState  setValue              setValueCallback
  setValueWithUndo        setZLevel               showControl
