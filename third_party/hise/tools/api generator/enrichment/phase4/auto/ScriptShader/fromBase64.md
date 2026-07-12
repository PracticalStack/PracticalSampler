Loads and compiles a shader from a base64-encoded string previously produced by `toBase64()`. This is the counterpart for distributing shader code without separate `.glsl` files - the encoded string contains the complete GLSL source including any `#include` dependencies, so the shader has no external file requirements.

The string is decoded, decompressed, and compiled as a fragment shader.
