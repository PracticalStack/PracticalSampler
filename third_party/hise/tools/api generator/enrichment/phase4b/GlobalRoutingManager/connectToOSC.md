GlobalRoutingManager::connectToOSC(JSON connectionData, Function errorFunction) -> Integer

Thread safety: UNSAFE -- creates OSC sender/receiver objects, performs network bind/connect,
allocates callback holders.
Establishes a bidirectional OSC connection for external controller communication. Calling
again with different settings tears down the previous connection first. Setting TargetPort
to -1 (or omitting it) creates a receive-only connection with no sender.
Callback signature: errorFunction(String errorMessage)

Required setup:
  const var rm = Engine.getGlobalRoutingManager();

Dispatch/mechanics:
  If data differs from lastData, tears down old sender/receiver
    -> creates HiseOSCReceiver(domain, sourcePort)
    -> if TargetPort != -1, creates HiseOSCSender and registers all cables as OSC targets
    -> rebuilds existing callback full addresses with the new domain

Pair with:
  addOSCCallback -- register script-level OSC message handlers
  sendOSCMessage -- send outbound OSC messages (requires TargetPort)
  getCable -- create /-prefixed cables for automatic OSC value routing

Anti-patterns:
  - [BUG] Always returns false regardless of whether the connection succeeded -- do not
    rely on the return value to check connection status
  - Do NOT omit TargetPort and then call sendOSCMessage -- no sender is created in
    receive-only mode, sendOSCMessage silently returns false

Source:
  ScriptingApiObjects.cpp:9008  GlobalRoutingManagerReference::connectToOSC()
    -> wraps errorFunction in WeakCallbackHolder
    -> m->connectToOSC(data)
  GlobalRoutingManager.cpp:587  GlobalRoutingManager::connectToOSC()
    -> creates HiseOSCReceiver, optionally HiseOSCSender
    -> notifies oscListeners
