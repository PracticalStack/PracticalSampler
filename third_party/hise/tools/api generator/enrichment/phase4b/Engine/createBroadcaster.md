Engine::createBroadcaster(var defaultValues) -> ScriptObject

Thread safety: UNSAFE -- heap allocation, JavascriptProcessor registration, metadata parsing
Creates a broadcaster that sends messages to attached listeners. The defaultValues
parameter defines the argument schema via a JSON object with id and args properties.
Required setup:
  const var bc = Engine.createBroadcaster({
      "id": "MyBroadcaster",
      "args": ["value", "source"]
  });
Pair with:
  Broadcaster.addListener -- attach listener callbacks
  Broadcaster.sendSyncMessage/sendAsyncMessage -- dispatch values
  Broadcaster.attachToComponentValue -- auto-wire to UI components
Source:
  ScriptingApi.cpp  Engine::createBroadcaster()
    -> new ScriptBroadcaster(defaultValues) -> registers as callable object
