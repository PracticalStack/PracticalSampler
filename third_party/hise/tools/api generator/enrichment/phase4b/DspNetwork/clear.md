DspNetwork::clear(Integer removeNodesFromSignalChain, Integer removeUnusedNodes) -> undefined

Thread safety: UNSAFE -- removes ValueTree children with undo manager notifications. When removeUnusedNodes is true, acquires MessageManagerLock inside the cleanup loop.
Removes nodes from the network. removeNodesFromSignalChain detaches all child nodes
and parameters from the root container's ValueTree. removeUnusedNodes deletes nodes
not in any signal path and triggers garbage collection of filter coefficient objects.
Required setup:
  const var nw = Engine.createDspNetwork("MyNetwork");
Pair with:
  create/createAndAdd -- to rebuild the graph after clearing
Anti-patterns:
  - Do NOT assume clear(true, false) fully removes nodes -- it detaches from the root
    but leaves them registered in the network's node list. Follow with clear(false, true)
    to remove orphaned nodes entirely.
Source:
  DspNetwork.cpp  clear()
    -> removeNodesFromSignalChain: removes ValueTree children from root node
    -> removeUnusedNodes: iterates registered nodes, calls deleteIfUnused() internally,
       acquires MessageManagerLock, triggers ExternalDataHolder garbage collection
