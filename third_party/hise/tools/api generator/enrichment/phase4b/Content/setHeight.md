Content::setHeight(int newHeight) -> undefined

Thread safety: SAFE -- simple integer comparison and assignment. The async broadcast is dispatched but does not block.
Sets the interface height in pixels. Only broadcasts a size change if the new value
differs from the current height AND the width is non-zero.

Dispatch/mechanics:
  Compares newHeight to current height
  If changed AND width != 0: interfaceSizeBroadcaster.sendMessage(async)

Pair with:
  setWidth -- must be set first (or use makeFrontInterface for both)
  getInterfaceSize -- read current dimensions

Anti-patterns:
  - If setHeight is called before setWidth (width is still 0), no size broadcast
    is sent. The resize only takes effect after width is also set.

Source:
  ScriptingApiContent.cpp:8099  Content::setHeight()
    -> conditional broadcast via interfaceSizeBroadcaster
