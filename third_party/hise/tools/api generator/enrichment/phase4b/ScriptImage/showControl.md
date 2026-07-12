ScriptImage::showControl(Integer shouldBeVisible) -> undefined

Thread safety: UNSAFE
Sets the visible property with change message notification.
1 = show, 0 = hide.
Pair with:
  fadeComponent -- for animated visibility transitions
Source:
  ScriptingApiContent.cpp  ScriptComponent::showControl()
    -> sets visible property with sendNotification
