ScriptShader::setEnableCachedBuffer(Integer shouldEnable) -> undefined

Thread safety: SAFE
Enables GPU-to-CPU buffer capture after each shader render via glReadPixels.
Required for Content.createScreenshot() to capture shader output. Adds
per-frame GPU readback overhead -- only enable when screenshot capture is needed.

Required setup:
  const var shd = Content.createShader("myEffect");
  shd.setEnableCachedBuffer(true);

Dispatch/mechanics:
  Sets enableCache flag. When true, addShader draw action calls glReadPixels
  after each render -> creates CachedImageBuffer (RGB) -> flips image
  vertically (GL Y-axis). For screenshots: prepareScreenshot() sets
  screenshotPending, blockWhileWaiting() polls up to 2000ms.

Pair with:
  Content.createScreenshot -- requires cached buffer to capture shader output

Source:
  ScriptingGraphics.cpp  ScriptShader::setEnableCachedBuffer()
    -> sets enableCache flag
  ScriptDrawActions.cpp:687  addShader draw action
    -> glReadPixels(GL_BGR_EXT, GL_UNSIGNED_BYTE) -> CachedImageBuffer
    -> vertical flip -> renderWasFinished(buffer)
