ScriptPanel::loadImage(String imageName, String prettyName) -> undefined

Thread safety: UNSAFE -- loads image resource, modifies internal image list
Loads an image from the project's Images pool and stores it with the given pretty
name alias. Images are loaded via the expansion handler, supporting both main
project and expansion pack images. Use the pretty name to reference the image
in paint routines or with setImage().
Required setup:
  const var pnl = Content.addPanel("Panel1", 0, 0);
  pnl.loadImage("{PROJECT_FOLDER}myImage.png", "myImage");
Pair with:
  isImageLoaded -- check if a named image is loaded
  unloadAllImages -- clear all loaded images
  setImage -- display a loaded image in filmstrip mode
Source:
  ScriptingApiContent.cpp  ScriptPanel::loadImage()
