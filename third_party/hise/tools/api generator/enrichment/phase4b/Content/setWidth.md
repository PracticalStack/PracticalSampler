Content::setWidth(int newWidth) -> undefined

Thread safety: SAFE -- simple integer comparison and assignment. The async broadcast is dispatched but does not block.
Sets the interface width in pixels. Only broadcasts a size change if the new value
differs from the current width AND the height is non-zero.

Dispatch/mechanics:
  Compares newWidth to current width
  If changed AND height != 0: interfaceSizeBroadcaster.sendMessage(async)

Pair with:
  setHeight -- must also be set (or use makeFrontInterface for both)
  getInterfaceSize -- read current dimensions

Source:
  ScriptingApiContent.cpp:8110  Content::setWidth()
    -> conditional broadcast via interfaceSizeBroadcaster
