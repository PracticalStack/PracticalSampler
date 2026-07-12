Content::makeFrontInterface(int width, int height) -> undefined

Thread safety: INIT -- calls addToFront(true) which registers this script as the front interface. Must be called during onInit.
Sets the interface dimensions and registers this script processor as the front interface.
Typically the first call in onInit for the main interface script. Sets both width and
height, broadcasts the size change, and calls addToFront(true).

Dispatch/mechanics:
  Sets width and height member variables
  interfaceSizeBroadcaster.sendMessage(sendNotificationAsync, width, height)
  JavascriptMidiProcessor::addToFront(true)

Pair with:
  getInterfaceSize -- read back current dimensions
  setWidth/setHeight -- change dimensions individually after init

Anti-patterns:
  - Do NOT call outside of onInit -- addToFront(true) must run during initialization.
  - Do NOT call both makeFrontInterface and makeFullScreenInterface -- they are
    mutually exclusive. Use one or the other.

Source:
  ScriptingApiContent.cpp:8131  Content::makeFrontInterface()
    -> sets width/height
    -> interfaceSizeBroadcaster.sendMessage(async)
    -> addToFront(true)
