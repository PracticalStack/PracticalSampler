ScriptPanel::isImageLoaded(String prettyName) -> Integer

Thread safety: SAFE
Returns 1 if an image with the given pretty name has been loaded via loadImage(),
0 otherwise. Checks the internal loaded images list.
Pair with:
  loadImage -- load an image with a pretty name
  unloadAllImages -- clear all loaded images
Source:
  ScriptingApiContent.cpp  ScriptPanel::isImageLoaded()
