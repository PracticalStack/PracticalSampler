## createShader

**Examples:**

```javascript:shader-panel-workflow
// Title: Creating and using an OpenGL shader for a ScriptPanel
// Context: GLSL fragment shaders provide GPU-accelerated visuals for
// visualizations, animated backgrounds, or complex rendering effects.
// The shader file lives in the project's Scripts/Shaders folder.

Content.makeFrontInterface(900, 600);

// Load the shader from the project's shader folder
const var shader = Content.createShader("visualizer");

// Configure the panel
const var displayPanel = Content.addPanel("DisplayPanel", 10, 10);
displayPanel.set("width", 400);
displayPanel.set("height", 300);

// Attach the shader to the panel
displayPanel.setShader(shader, []);

// Pass dynamic data to the shader via uniforms
displayPanel.setTimerCallback(function()
{
    // Update uniform values that the shader reads each frame
    shader.setUniformData("intensity", [this.data.level]);
    this.repaint();
});

displayPanel.startTimer(30);  // ~33fps
```
```json:testMetadata:shader-panel-workflow
{
  "testable": false,
  "skipReason": "Requires a shader file in the project's shader folder"
}
```

Pass an empty string to `createShader("")` to create a shader object without loading a file. You can then set the fragment shader code programmatically via `ScriptShader.setFragmentShader()`.
