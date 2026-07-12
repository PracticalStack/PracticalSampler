Colours::toVec4(Colour colour) -> Array

Thread safety: SAFE
Converts a colour to [r, g, b, a] where each component is a 0.0-1.0 float.
Output format is compatible with GLSL vec4 uniforms via ScriptShader.setUniformData.

Dispatch/mechanics:
  Parses colour via getCleanedObjectColour
  Returns [getFloatRed(), getFloatGreen(), getFloatBlue(), getFloatAlpha()]

Pair with:
  fromVec4 -- inverse operation, lossless roundtrip

Source:
  ScriptingApi.cpp:7038  ScriptingApi::Colours::toVec4()
    -> getCleanedObjectColour(colour)
    -> Array<var>{getFloatRed(), getFloatGreen(), getFloatBlue(), getFloatAlpha()}
