ScriptLookAndFeel::isImageLoaded(String prettyName) -> Integer

Thread safety: WARNING -- String parameter involvement, atomic ref-count operations on var-to-String conversion.
Returns true if an image with the given alias has been loaded via loadImage(). Performs
a linear search through the loaded images list comparing prettyName strings.

Dispatch/mechanics:
  Linear search through loadedImages array comparing prettyName strings.
  Returns 1 if found, 0 if not.

Pair with:
  loadImage -- load the image before checking
  unloadAllImages -- clears all loaded images (isImageLoaded returns false after)

Source:
  ScriptingGraphics.cpp:6160  ScriptedLookAndFeel::isImageLoaded()
    -> linear search through loadedImages by prettyName
