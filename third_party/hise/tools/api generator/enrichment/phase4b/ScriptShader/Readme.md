ScriptShader (object)
Obtain via: Content.createShader(shaderFileName)

OpenGL fragment shader wrapper for GPU-accelerated visual effects within a
ScriptPanel paint routine. Provides Shadertoy-compatible built-in uniforms
(iTime, iResolution, fragCoord), custom uniform passing, blend modes,
preprocessor variants, and cached buffer support for screenshots.

Constants:
  BlendMode:
    GL_ZERO = 0                      Factor (0, 0, 0, 0)
    GL_ONE = 1                       Factor (1, 1, 1, 1)
    GL_SRC_COLOR = 768               Source color
    GL_ONE_MINUS_SRC_COLOR = 769     1 - source color
    GL_SRC_ALPHA = 770               Source alpha
    GL_ONE_MINUS_SRC_ALPHA = 771     1 - source alpha
    GL_DST_ALPHA = 772               Destination alpha
    GL_ONE_MINUS_DST_ALPHA = 773     1 - destination alpha
    GL_DST_COLOR = 774               Destination color
    GL_ONE_MINUS_DST_COLOR = 775     1 - destination color
    GL_SRC_ALPHA_SATURATE = 776      min(As, 1-Ad)

Complexity tiers:
  1. Static effect: Content.createShader, g.applyShader. GPU-rendered background
     with no uniforms or animation.
  2. Animated effect: + panel timer calling repaint(). Built-in iTime uniform
     drives time-based GLSL animations automatically.
  3. Data-driven visualization: + setUniformData, setBlendFunc, setPreprocessor.
     Stream app state to GPU via float/Buffer uniforms, compositing control,
     quality variants.
  4. Full pipeline: + setEnableCachedBuffer. Multiple uniforms per frame,
     additive blending, preprocessor quality tiers, screenshot support.

Practical defaults:
  - Use a 30ms timer interval for shader animations -- smooth ~33fps without
    excessive CPU overhead from uniform uploads.
  - Use setBlendFunc(true, shd.GL_SRC_ALPHA, shd.GL_ONE) for additive glow
    blending when compositing over existing panel content.
  - Only enable setEnableCachedBuffer(true) when screenshot capture is needed --
    per-frame GPU readback adds measurable overhead.
  - Use this.getLocalBounds(0) as the area argument to g.applyShader() to fill
    the entire panel.

Common mistakes:
  - Using g.applyShader() without OpenGL enabled -- shader cannot compile or
    render, area remains blank with no error in frontend builds.
  - Expecting iMouse to update automatically -- must call
    setUniformData("iMouse", [x, y]) from the panel's mouse callback.
  - Calling setPreprocessor in a timer without change detection -- every call
    recompiles the shader on the GPU, destroying performance at 30ms intervals.
  - Sending a raw Array as a large uniform -- arrays with more than 4 elements
    are silently ignored. Use Buffer for variable-length float arrays.
  - Creating one shader and switching its source at runtime -- shader compilation
    is expensive. Pre-create separate shader objects per mode and swap in the
    paint routine.

Example:
  const var pnl = Content.addPanel("ShaderPanel", 0, 0);
  pnl.set("width", 512);
  pnl.set("height", 512);

  const var shd = Content.createShader("myEffect");

  pnl.setPaintRoutine(function(g)
  {
      g.applyShader(shd, [0, 0, this.getWidth(), this.getHeight()]);
  });

  pnl.startTimer(30);
  pnl.setTimerCallback(function()
  {
      this.repaint();
  });

Methods (8):
  fromBase64                getOpenGLStatistics
  setBlendFunc              setEnableCachedBuffer
  setFragmentShader         setPreprocessor
  setUniformData            toBase64
