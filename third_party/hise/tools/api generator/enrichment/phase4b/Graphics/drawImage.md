Graphics::drawImage(String imageName, Array area, Number xOffset, Number yOffset) -> undefined

Thread safety: UNSAFE -- allocates new draw actions, resolves image by name from parent panel or LAF
Draws a previously loaded image into the area. Images must be loaded via
ScriptPanel.loadImage() or ScriptLookAndFeel.loadImage() first. Only works in
ScriptPanel paint routines or ScriptedLookAndFeel drawing functions.
yOffset selects vertical frame for filmstrip animations. xOffset is ignored.

Required setup:
  Panel1.loadImage("knob.png", "knob.png");

Anti-patterns:
  - xOffset parameter is silently ignored (declared as int /*xOffset*/ in C++)
  - Only works in ScriptPanel/ScriptedLookAndFeel contexts -- other contexts
    trigger "drawImage is only allowed in a panel's paint routine"
  - When image not found, a grey "XXX" placeholder is drawn AND the current
    colour state is overwritten (set to grey then black). Call setColour again
    after a potentially failed drawImage.

Source:
  ScriptingGraphics.cpp  GraphicsObject::drawImage()
    -> dynamic_cast to ScriptPanel or ScriptedLookAndFeel to get loaded image
    -> image scaled to area width, yOffset selects vertical frame
    -> if not found: grey placeholder + debugError in backend builds
