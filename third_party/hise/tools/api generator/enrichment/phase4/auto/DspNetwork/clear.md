Removes nodes from the network. The two flags operate independently: `removeNodesFromSignalChain` detaches all child nodes and parameters from the root container, while `removeUnusedNodes` deletes any nodes not currently in a signal path and triggers garbage collection. Both flags accept `0` or `1`.

> [!Warning:Flags are independent] Calling `clear(true, false)` detaches nodes from the root container but leaves them registered in the network as orphaned objects. A subsequent `clear(false, true)` is needed to remove them entirely. To fully reset the network, pass `clear(true, true)`.
