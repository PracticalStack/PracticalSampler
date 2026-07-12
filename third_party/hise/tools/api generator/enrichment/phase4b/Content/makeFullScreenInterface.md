Content::makeFullScreenInterface() -> undefined

Thread safety: INIT -- calls addToFront(true). Must be called during onInit.
Sets the interface dimensions to the device simulator's display resolution and
registers this script as the front interface. Uses HiseDeviceSimulator::getDisplayResolution()
to determine the size.

Dispatch/mechanics:
  HiseDeviceSimulator::getDisplayResolution() -> sets width/height
  addToFront(true)

Pair with:
  makeFrontInterface -- for explicit dimension control instead of full-screen

Source:
  ScriptingApiContent.cpp:8142  Content::makeFullScreenInterface()
    -> HiseDeviceSimulator::getDisplayResolution()
    -> addToFront(true)
