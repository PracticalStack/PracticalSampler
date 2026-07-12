Loads a GLSL fragment shader file from the Scripts folder and compiles it. Pass the filename without the `.glsl` extension. The loaded code should contain a `main()` function - the engine automatically prepends built-in uniforms, coordinate macros, and any preprocessor definitions set via `setPreprocessor`.

GLSL files support `#include "otherFile"` directives for splitting shader code across multiple files. Includes are resolved recursively, and circular dependencies are detected and throw an error.

In the HISE backend, the code editor provides a dropdown to select shader files directly. If the specified file does not exist, a default Shadertoy-style template is created. The file is registered with the file watcher for live reloading during development.

Note that `Content.createShader("myShader")` calls this method internally, so you typically only need `setFragmentShader` when switching a shader object to a different source file.
