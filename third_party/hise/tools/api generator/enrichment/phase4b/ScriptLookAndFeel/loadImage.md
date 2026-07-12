ScriptLookAndFeel::loadImage(String imageName, String prettyName) -> undefined

Thread safety: UNSAFE
Loads an image from the project's Images folder (or expansion pack) and stores it under
the given alias. The alias is used to reference the image in paint functions via
g.drawImage(). If prettyName already exists and the file reference differs, the image is
silently replaced.

Required setup:
  const var laf = Content.createLocalLookAndFeel();
  laf.loadImage("{PROJECT_FOLDER}myBg.png", "background");

Dispatch/mechanics:
  Creates PoolReference from imageName (ProjectHandler::SubDirectories::Images)
    -> ExpansionHandler::loadImageReference() (supports {EXP::Name} prefix)
    -> stores NamedImage{image, prettyName} in loadedImages array
    -> uses TimeoutExtender to prevent script timeout during disk I/O

Pair with:
  isImageLoaded -- verify the image was found after loading
  unloadAllImages -- release all loaded image references

Anti-patterns:
  - Do NOT assume loading succeeded -- if the image file is not found, only a console
    warning is printed (no script error). Use isImageLoaded() after loading to verify.

Source:
  ScriptingGraphics.cpp:6126  ScriptedLookAndFeel::loadImage()
    -> PoolReference creation -> ExpansionHandler::loadImageReference()
    -> TimeoutExtender for disk I/O safety
