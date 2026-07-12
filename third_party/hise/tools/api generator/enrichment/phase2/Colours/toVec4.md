## toVec4

**Examples:**

```javascript:glsl-uniform
// Title: Passing a colour to a GLSL shader as a vec4 uniform
// Context: When using ScriptShader for GPU-rendered visuals, shader
// uniforms expect float arrays. toVec4 converts an ARGB integer
// colour into the [r, g, b, a] float format that GLSL vec4 requires.

const var panel = Content.addPanel("ShaderPanel", 0, 0);
panel.set("width", 400);
panel.set("height", 400);

const var shader = Content.createShader("myShader");

// Convert a theme colour and pass it to the shader
var accentColour = Colours.withAlpha(0xFF94F2FF, 0.8);
shader.setUniformData("uColour", Colours.toVec4(accentColour));

// The shader receives it as: uniform vec4 uColour; // [r, g, b, a] in 0.0-1.0
```
```json:testMetadata:glsl-uniform
{
  "testable": false,
  "skipReason": "Requires ScriptShader context"
}
```

```javascript:inspect-channels
// Title: Inspecting colour channel values
// Context: toVec4 is useful for debugging or computing with individual
// colour channels as normalized floats.

var rgba = Colours.toVec4(Colours.dodgerblue);

Console.print("R: " + rgba[0]); // R: 0.118 (approx)
Console.print("G: " + rgba[1]); // G: 0.565 (approx)
Console.print("B: " + rgba[2]); // B: 1.0
Console.print("A: " + rgba[3]); // A: 1.0
```
```json:testMetadata:inspect-channels
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "rgba[2]", "value": 1.0},
    {"type": "REPL", "expression": "rgba[3]", "value": 1.0},
    {"type": "REPL", "expression": "rgba.length", "value": 4}
  ]
}
```
