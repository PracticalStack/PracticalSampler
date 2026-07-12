ScriptPanel::setImage(String imageName, Integer xOffset, Integer yOffset) -> undefined

Thread safety: UNSAFE -- switches to fixed image mode, clears paint routine
Switches to fixed image mode, bypassing the paint routine. Clips a region from a
previously loaded image (via loadImage()) for filmstrip-style rendering. Either
xOffset or yOffset must be 0 -- the other selects the frame along the strip.
Required setup:
  const var pnl = Content.addPanel("Panel1", 0, 0);
  pnl.loadImage("{PROJECT_FOLDER}strip.png", "strip");
Anti-patterns:
  - Calling setImage() clears any previously set paint routine -- the panel switches
    to fixed image mode until a new paint routine is set via setPaintRoutine()
  - Either xOffset or yOffset must be 0 -- both non-zero is invalid
Pair with:
  loadImage -- must load the image first
  setPaintRoutine -- to switch back from fixed image mode to custom drawing
Source:
  ScriptingApiContent.cpp  ScriptPanel::setImage()
    -> sets usesClippedFixedImage = true, clears paint routine
