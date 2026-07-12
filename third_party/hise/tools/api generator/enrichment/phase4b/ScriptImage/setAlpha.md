ScriptImage::setAlpha(Double newAlphaValue) -> undefined

Thread safety: UNSAFE
Sets the transparency of the displayed image. Clamped to 0.0-1.0 during rendering
(0.0 = fully transparent, 1.0 = fully opaque). Triggers a UI repaint.
Pair with:
  fadeComponent -- for animated visibility transitions (setAlpha is instant)
Source:
  ScriptingApiContent.cpp:4186  ScriptImage::setAlpha()
    -> setScriptObjectPropertyWithChangeMessage(Alpha, newAlphaValue)
    -> rendering clamps via jmax/jmin in ImageComponentWithMouseCallback::paint()
