# ScriptShader -- Project Context

## Project Context

### Real-World Use Cases
- **Music visualization viewport**: A piano plugin uses multiple GLSL shaders to render real-time note visualizations - waterfall displays of played notes, particle-based grain effects, and harmonic analysis wheels. Each visualization mode gets its own shader, hosted in a dedicated ScriptPanel, with note/velocity data streamed to the GPU via Buffer uniforms on a 30ms timer.
- **Data-driven GPU rendering**: Shader uniforms are fed by a FixObjectFactory stack pipeline - note events are tracked in typed stacks, their properties bulk-copied into Buffers via `stack.copy("property", buffer)`, and the Buffers sent to the GPU with `setUniformData`. This decouples the data model from the rendering code.
- **Multi-quality shader variants**: A graphics quality setting drives `setPreprocessor("GRAPHICS_LEVEL", level)` across all active shaders simultaneously, enabling `#ifdef`-based quality tiers in the GLSL code without reloading shader files.

### Complexity Tiers
1. **Static effect** (simplest): `Content.createShader()` + `g.applyShader()` in a paint routine. No uniforms, no animation. Just a GPU-rendered background or overlay.
2. **Animated effect**: Add a 30ms timer that calls `repaint()` each tick. The built-in `iTime` uniform drives time-based GLSL animations automatically.
3. **Data-driven visualization**: Stream application state to the shader via `setUniformData` - float scalars for simple parameters, Buffer arrays for per-note or per-element data. Combine with `setBlendFunc` for compositing control and `setPreprocessor` for quality variants.
4. **Full pipeline** (most complex): FixObjectFactory stacks as the data model, bulk `stack.copy()` into Buffers, multiple uniforms per frame, additive blending, preprocessor-based quality tiers, and `setEnableCachedBuffer` for screenshot support.

### Practical Defaults
- Use a 30ms timer interval for shader animations - this provides smooth ~33fps updates without excessive CPU overhead from uniform uploads.
- Use `setBlendFunc(true, shd.GL_SRC_ALPHA, shd.GL_ONE)` for additive (glow) blending when compositing shader output over existing panel content.
- Only enable `setEnableCachedBuffer(true)` when screenshot capture is needed - the per-frame GPU readback adds measurable overhead.
- Use `this.getLocalBounds(0)` as the area argument to `g.applyShader()` to fill the entire panel.

### Integration Patterns
- `FixObjectFactory.createStack()` -> `stack.copy("prop", buffer)` -> `ScriptShader.setUniformData("prop", buffer)` - Typed data stacks as the source for GPU uniform arrays. The stack tracks structured events (notes, particles), and `copy()` extracts a single property column into a Buffer for GPU upload.
- `ScriptPanel.setTimerCallback()` -> `ScriptShader.setUniformData()` -> `ScriptPanel.repaint()` - The standard animation loop. A 30ms timer updates uniform data and triggers a repaint, which runs the paint routine containing `g.applyShader()`.
- `GlobalCable.getValue()` -> `ScriptShader.setUniformData()` - DSP network values (envelope levels, peak meters) read via GlobalRoutingManager cables and forwarded to the shader as float uniforms for audio-reactive visualization.
- `Colours.toVec4(colour)` -> `ScriptShader.setUniformData("color", vec4)` - Convert HISE colour values to vec4 arrays for GLSL consumption.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Calling `setPreprocessor` in the timer callback without change detection | Guard with `if (newLevel != currentLevel)` and call `setPreprocessor` only on change | Every `setPreprocessor` call recompiles the shader on the GPU. Calling it every 30ms frame destroys performance. |
| Sending a raw Array as a large uniform | Use `Buffer.create(size)` and pass the Buffer | Arrays with more than 4 elements are silently ignored. Buffer is the correct type for variable-length float arrays in GLSL. |
| Creating one shader and switching its source at runtime | Create separate shader objects per visualization mode and swap which one `g.applyShader()` renders | Shader compilation is expensive. Pre-create all variants and switch between them in the paint routine. |
