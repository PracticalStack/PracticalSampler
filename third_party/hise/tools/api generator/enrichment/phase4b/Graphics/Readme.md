Graphics (object)
Obtain via: Passed as the first argument to ScriptPanel paint callbacks and ScriptLookAndFeel drawing functions. Not user-created.

2D drawing context for paint callbacks -- shapes, text, images, paths, layers,
and post-processing effects. All draw calls are recorded as deferred actions on
the scripting thread and replayed on the UI thread via a command list architecture.

Complexity tiers:
  1. Basic rendering: fillAll, setColour, fillRect, fillRoundedRectangle,
     setFont/setFontWithSpacing, drawAlignedText. Simple button and panel
     backgrounds with text labels.
  2. Path-based controls: + fillPath, drawPath, fillEllipse, drawEllipse,
     drawDropShadow, fillTriangle, setGradientFill. Arc-based rotary knobs,
     icon buttons, slider tracks with gradients.
  3. Compositing and effects: + beginLayer/endLayer, addDropShadowFromAlpha,
     gaussianBlur/boxBlur, drawImage, rotate. Glowing elements, blurred
     backgrounds, filmstrip rendering, rotary knob position indicators.

Practical defaults:
  - Use Rectangle(obj.area) or Rectangle(this.getLocalBounds(0)) at the top of
    paint callbacks and LAF functions. Use removeFromTop, removeFromLeft, reduced,
    withSizeKeepingCentre, and other Rectangle methods for layout calculations
    instead of manual [x,y,w,h] array index arithmetic.
  - Use setFontWithSpacing over setFont in LAF callbacks -- the extra spacing
    parameter gives fine control over letter-spacing for compact labels.
  - Pre-compute colour constants at init time rather than calling
    Colours.withAlpha() inside every paint callback.
  - Use fillAll(colour) as the first call rather than setColour + fillRect --
    cleaner and does not require a prior setColour call.
  - Use Colours.mix(baseColour, Colours.white, obj.hover * 0.15) as the
    standard hover highlight pattern.

Common mistakes:
  - Passing area as separate arguments g.fillRect(10, 10, 100, 50) instead of
    an array g.fillRect([10, 10, 100, 50]) -- caught by backend diagnostics.
  - Using manual array arithmetic [area[0]+10, area[1], area[2]-20, area[3]]
    instead of Rectangle(area).reduced(10, 0) -- all area parameters accept
    Rectangle objects. Use Rectangle methods for layout calculations.
  - Calling a drawing method without setColour or setGradientFill first --
    colour is undefined, backend warns at parse time.
  - Calling post-processing (gaussianBlur, desaturate, applyHSL, etc.) without
    an active layer -- triggers script error. Wrap in beginLayer/endLayer.
  - Using "center" instead of "centred" for alignment strings -- British
    spelling required, invalid strings cause a script error.
  - Using g.drawLine(x1, y1, x2, y2, t) point-by-point order -- the actual
    parameter order is (x1, x2, y1, y2, t), grouping x-coords then y-coords.
  - Calling setColour once at the top of a complex LAF callback -- colour state
    persists, must be reset before each group of differently-coloured draws.

Example:
  // Graphics is not user-created -- it arrives as a paint callback parameter
  Panel1.setPaintRoutine(function(g)
  {
      var rect = Rectangle(this.getLocalBounds(0));
      g.fillAll(0xFF222222);

      var header = rect.removeFromTop(30);
      g.setColour(0xFFFFFFFF);
      g.setFont("Arial", 16.0);
      g.drawAlignedText("Hello", header.reduced(10, 0), "left");

      g.setColour(0x20FFFFFF);
      g.fillRoundedRectangle(rect.reduced(5), 3.0);
  });

Methods (49):
  addDropShadowFromAlpha      addNoise
  applyGamma                  applyGradientMap
  applyHSL                    applyMask
  applySepia                  applyShader
  applySharpness              applyVignette
  beginBlendLayer             beginLayer
  boxBlur                     desaturate
  drawAlignedText             drawAlignedTextShadow
  drawDropShadow              drawDropShadowFromPath
  drawEllipse                 drawFFTSpectrum
  drawHorizontalLine          drawImage
  drawInnerShadowFromPath     drawLine
  drawMarkdownText            drawMultiLineText
  drawPath                    drawRect
  drawRepaintMarker           drawRoundedRectangle
  drawSVG                     drawTriangle
  drawVerticalLine            endLayer
  fillAll                     fillEllipse
  fillPath                    fillRect
  fillRoundedRectangle        fillTriangle
  flip                        gaussianBlur
  getStringWidth              rotate
  setColour                   setFont
  setFontWithSpacing          setGradientFill
  setOpacity
