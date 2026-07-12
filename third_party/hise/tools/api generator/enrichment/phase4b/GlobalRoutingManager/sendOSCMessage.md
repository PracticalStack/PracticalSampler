GlobalRoutingManager::sendOSCMessage(String oscSubAddress, NotUndefined data) -> Integer

Thread safety: UNSAFE -- constructs OSC message objects and performs network I/O via JUCE
OSCSender.
Sends an OSC message to the configured target. The sub-address is appended to the connection
domain. Type conversion: doubles become float32, integers/booleans become int32, strings
become OSC strings. An array sends a multi-argument message with one argument per element.

Required setup:
  const var rm = Engine.getGlobalRoutingManager();
  rm.connectToOSC({"SourcePort": 9000, "TargetPort": 9001}, false);

Dispatch/mechanics:
  Gets HiseOSCSender -> constructs OSCAddressPattern(domain + subAddress)
    -> converts var data to OSC arguments (double->float32, int/bool->int32, string->string)
    -> handles both single values and arrays
    -> sender.send(message)

Pair with:
  connectToOSC -- must be called first with a valid TargetPort to create the sender
  addOSCCallback -- to receive responses from the OSC target

Anti-patterns:
  - Returns false with no error if no OSC sender is configured -- ensure connectToOSC was
    called with a valid TargetPort (not -1) before sending
  - Do NOT pass unsupported types (objects, arrays of objects) -- throws "illegal var type
    for OSC data"

Source:
  GlobalRoutingManager.cpp:692  sendOSCMessageToOutput()
    -> constructs OSCAddressPattern(domain + subAddress)
    -> converts var to OSC args
    -> sender->send(m)
