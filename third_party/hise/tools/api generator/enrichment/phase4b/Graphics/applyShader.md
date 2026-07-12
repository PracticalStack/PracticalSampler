Graphics::applyShader(ScriptObject shader, Array area) -> Integer

Thread safety: UNSAFE -- allocates a new draw action and accesses OpenGL context state
Applies an OpenGL shader to the specified area. Returns 1 if the shader was valid
and the action was queued, 0 if the shader parameter was not a valid ScriptShader.
Does NOT require an active layer.

Dispatch/mechanics:
  Draw action queued -> on UI thread: compiles shader (if dirty), sets up
  OpenGL blending and coordinate bounds, renders shader output into area

Anti-patterns:
  - Return value of 1 only means the action was enqueued -- actual compilation errors
    are deferred to the render pass (logged in backend builds only)
  - Returns 0 silently for invalid shader objects -- no error message produced
  - Requires OpenGL to be enabled; without it, backend logs "Open GL is not enabled"

Source:
  ScriptingGraphics.cpp  GraphicsObject::applyShader()
    -> new draw action stores ScriptShader var reference
    -> perform() compiles shader, configures OpenGL, renders into area
