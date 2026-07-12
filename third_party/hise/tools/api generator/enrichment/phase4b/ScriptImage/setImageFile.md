ScriptImage::setImageFile(String absoluteFileName, Integer forceUseRealFile) -> undefined

Thread safety: UNSAFE
Sets the image file to display. Resolved via PoolReference with the project's
Images/ folder. Supports expansion images via ExpansionHandler. Pass empty string
to clear. Recomputes blend mode after loading.
Required setup:
  const var img = Content.addImage("MyImage", 0, 0);
Dispatch/mechanics:
  PoolReference(absoluteFileName, Images/) -> ExpansionHandler::loadImageReference()
    -> stores as PooledImage -> updateBlendMode() if blendMode != Normal
    -> sets FileName property with notification
Pair with:
  set("fileName", path) -- equivalent alternative via property API
Anti-patterns:
  - [BUG] forceUseRealFile parameter is ignored (ignoreUnused in C++) -- images always
    load through the pool/expansion handler regardless of this value
Source:
  ScriptingApiContent.cpp:4132  ScriptImage::setImageFile()
    -> PoolReference -> ExpansionHandler::loadImageReference(ref)
    -> updateBlendMode() -> setScriptObjectProperty(FileName)
