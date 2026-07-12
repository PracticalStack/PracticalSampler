## setBlendFunc

**Examples:**

```javascript:additive-glow-trails
// Title: Additive blending for note glow trails
// Context: A note visualization shader renders bright trails over a dark
//          background. Additive blending makes overlapping notes glow
//          brighter instead of occluding each other.

const var shd = Content.createShader("noteTrails");

// Additive blend: output = src * srcAlpha + dst * 1
// This adds the shader's RGB output to whatever is already drawn,
// creating a natural glow where bright areas overlap.
shd.setBlendFunc(true, shd.GL_SRC_ALPHA, shd.GL_ONE);

const var pnl = Content.getComponent("TrailPanel");

pnl.setPaintRoutine(function(g)
{
    // Draw background first (e.g., dark gradient or image)
    g.fillAll(0xFF101020);

    // Shader output is additively blended on top
    g.applyShader(shd, this.getLocalBounds(0));
});
```
```json:testMetadata:additive-glow-trails
{
  "testable": false,
  "skipReason": "Requires shader file and active OpenGL context for rendering"
}
```
