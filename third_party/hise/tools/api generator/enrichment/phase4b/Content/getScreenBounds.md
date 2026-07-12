Content::getScreenBounds(Integer getTotalArea) -> Array

Thread safety: UNSAFE -- acquires MessageManagerLock, blocks if not on the message thread.
Returns the screen bounds as an [x, y, width, height] array. When getTotalArea is true,
returns the total display area including taskbar/dock. When false, returns only the
user-available area (excluding taskbar).

Dispatch/mechanics:
  Acquires MessageManagerLock
  Desktop::getDisplays() -> main display
  getTotalArea ? totalArea : userArea
  Returns [x, y, width, height]

Anti-patterns:
  - Do NOT call from the audio thread -- acquires MessageManagerLock, which blocks
    until the message thread is free, causing audio dropouts.

Source:
  ScriptingApiContent.cpp:7987  Content::getScreenBounds()
    -> MessageManagerLock acquisition
    -> Desktop::getDisplays().getMainDisplay()
