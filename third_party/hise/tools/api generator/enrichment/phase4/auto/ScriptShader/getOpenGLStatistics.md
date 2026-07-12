Returns a JSON object with information about the GPU and OpenGL context. The returned object contains these properties:

| Property | Type | Description |
|----------|------|-------------|
| `VersionString` | String | Full GL version string |
| `Major` | int | GL major version number |
| `Minor` | int | GL minor version number |
| `Vendor` | String | GPU vendor name |
| `Renderer` | String | GPU model name |
| `GLSL Version` | String | Supported shader language version |

If no OpenGL context is active, the properties contain placeholder values ("Inactive" and zero).

> [!Warning:Only available after first render] The statistics object is populated during the first GPU compilation pass on the render thread. Calling this before the shader has been rendered at least once returns `undefined`.
