Engine::getPlayHead() -> JSON

Thread safety: UNSAFE -- returns heap-allocated DynamicObject pointer
Returns host transport info object. WARNING: the property-population code in
MainController::setHostBpm() is entirely commented out -- returns empty object.
Use createTransportHandler() or getHostBpm() instead.
Anti-patterns:
  - Do NOT use getPlayHead() properties -- all are undefined. Use createTransportHandler().
Source:
  ScriptingApi.cpp  Engine::getPlayHead()
    -> returns MainController::hostInfo (empty DynamicObject)
