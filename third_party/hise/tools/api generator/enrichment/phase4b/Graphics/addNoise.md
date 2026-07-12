Graphics::addNoise(NotUndefined noiseAmount) -> undefined

Thread safety: UNSAFE -- allocates a new draw action and accesses a shared NoiseMapManager
Adds a noise texture overlay. Accepts a simple float (0.0-1.0 opacity) or a JSON
object with alpha, monochromatic, scaleFactor, and area properties.

Dispatch/mechanics:
  NoiseMapManager (SharedResourcePointer) caches noise images by parameters
  -> avoids regenerating noise texture every paint call
  -> draw action composites cached noise image onto canvas

Anti-patterns:
  - Do NOT use the simple float form inside ScriptedLookAndFeel callbacks -- the parent
    may not be a ScriptComponent, producing "No valid area for noise map specified".
    Use the JSON form with an explicit area property instead.
  - scaleFactor of -1.0 is a sentinel for auto-detect; other negative values clamp to 0.125

Source:
  ScriptingGraphics.cpp  GraphicsObject::addNoise()
    -> DrawActions::NoiseMapManager (SharedResourcePointer) for cached noise images
    -> JSON parsing for alpha, monochromatic, scaleFactor, area properties
