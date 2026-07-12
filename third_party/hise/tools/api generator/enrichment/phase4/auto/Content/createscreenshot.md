Captures a section of the interface and saves it as a PNG file. The `area` parameter accepts either a `[x, y, w, h]` array for arbitrary coordinates or a component reference to capture that component's bounds. Pass a `File` object pointing to the target directory and a filename without the `.png` extension.

Visual guides added with `addVisualGuide()` are automatically hidden during capture.

> [!Warning:UI zoom affects output dimensions] The current UI zoom factor affects the output dimensions. At 200% zoom, the resulting image is twice the default interface size. OpenGL shaders are not captured by default - call `ScriptShader.setEnableCachedBuffer()` to include them in screenshots.