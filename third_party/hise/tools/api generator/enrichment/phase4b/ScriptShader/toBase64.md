ScriptShader::toBase64() -> String

Thread safety: UNSAFE
Returns the current shader source as a base64-encoded, zstd-compressed string.
Captures raw user code (without auto-prepended header and preprocessor
definitions) for embedding directly in scripts without separate .glsl files.

Pair with:
  fromBase64 -- restore a shader from the encoded string
  setFragmentShader -- must load a shader first to have source to encode

Source:
  ScriptingGraphics.cpp  ScriptShader::toBase64()
    -> reads compiledCode (raw user code) -> zstd compress -> Base64 encode
