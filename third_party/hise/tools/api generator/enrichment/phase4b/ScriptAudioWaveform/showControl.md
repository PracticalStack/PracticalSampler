ScriptAudioWaveform::showControl(Integer shouldBeVisible) -> undefined

Thread safety: UNSAFE
Sets the visible property with change message notification.
Pass 1 to show, 0 to hide.

Pair with:
  fadeComponent -- for animated visibility transitions

Source:
  ScriptingApiContent.cpp  ScriptComponent::showControl()
    -> sets visible property with sendNotification
