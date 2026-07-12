Controls how the shader output is composited with the existing framebuffer content using OpenGL alpha blending. The first parameter enables or disables blending; the other two specify source and destination blend factors using the `GL_*` constants on the shader object.

When blending is enabled, the previous GL blend state is saved before rendering and restored afterward. The default blend factors (before any call) are `GL_SRC_ALPHA` / `GL_ONE_MINUS_SRC_ALPHA`.

Common blend mode recipes:

```javascript
const var shd = Content.createShader("myShader");

// No blending (fully opaque, overwrites background)
shd.setBlendFunc(false, shd.GL_ZERO, shd.GL_ZERO);

// Standard alpha blending (default)
shd.setBlendFunc(true, shd.GL_SRC_ALPHA, shd.GL_ONE_MINUS_SRC_ALPHA);

// Additive blending with alpha (glow effect)
shd.setBlendFunc(true, shd.GL_SRC_ALPHA, shd.GL_ONE);

// Additive blending without alpha
shd.setBlendFunc(true, shd.GL_ONE, shd.GL_ONE);
```

See [learnopengl.com/Advanced-OpenGL/Blending](https://learnopengl.com/Advanced-OpenGL/Blending) for a detailed explanation of blend factor mathematics.
