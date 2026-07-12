ScriptLookAndFeel::unloadAllImages() -> undefined

Thread safety: UNSAFE
Removes all images previously loaded via loadImage(), releasing their pooled references.
After calling this, isImageLoaded() returns false for all previously loaded aliases and
paint functions can no longer reference the unloaded images.

Pair with:
  loadImage -- load images that this method releases
  isImageLoaded -- verify image state after unloading

Source:
  ScriptingGraphics.cpp:6155  ScriptedLookAndFeel::unloadAllImages()
    -> loadedImages.clear()
