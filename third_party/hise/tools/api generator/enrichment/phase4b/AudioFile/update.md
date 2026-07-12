AudioFile::update() -> undefined

Thread safety: SAFE -- posts an asynchronous notification only, no allocation or lock.
Sends an asynchronous content change notification to all registered listeners
and content callbacks. Call after modifying audio data directly through Buffer
objects obtained from getContent to trigger UI updates.

Dispatch/mechanics:
  buffer->getUpdater().sendContentChangeMessage(sendNotificationAsync, -1)
    -> triggers all content listeners (C++ Listeners and script contentCallbacks)

Pair with:
  getContent -- obtain Buffer objects for direct modification
  setContentCallback -- register to receive the notification this method sends

Source:
  ScriptingApiObjects.cpp:1770  ScriptAudioFile::update()
    -> buffer->getUpdater().sendContentChangeMessage(sendNotificationAsync, -1)
