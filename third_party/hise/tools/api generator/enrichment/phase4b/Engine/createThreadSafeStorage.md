Engine::createThreadSafeStorage() -> ScriptObject

Thread safety: UNSAFE -- heap allocation
Creates a thread-safe storage container for lock-free data exchange between the audio
thread and scripting/UI thread. Avoids the need for locks or unsafe shared variable
access across threads.
Source:
  ScriptingApi.cpp  Engine::createThreadSafeStorage()
    -> new ScriptThreadSafeStorage
