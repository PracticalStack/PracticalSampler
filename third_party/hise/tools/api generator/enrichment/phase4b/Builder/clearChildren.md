Builder::clearChildren(Integer buildIndex, Integer chainIndex) -> undefined

Thread safety: UNSAFE -- acquires MessageManagerLock for each processor removal.
  Performs heap deallocation via Chain handler remove.
Removes all child processors from a specific chain of the module at buildIndex.
Use ChainIndexes constants for chainIndex. Pass ChainIndexes.Direct (-1) to
treat the module itself as the chain (for removing sound generators from a
container).

Required setup:
  const var b = Synth.createBuilder();
  var idx = b.getExisting("MyContainer");

Dispatch/mechanics:
  createdModules[buildIndex] -> resolve Chain (direct or child at chainIndex)
    -> iterate chain handler -> sendDeleteMessage() + handler.remove() per child

Pair with:
  create -- rebuild chain contents after clearing
  flush -- finalize after modifications

Anti-patterns:
  - Do NOT target a chain containing the calling script processor -- unlike
    clear(), clearChildren() has no self-preservation check. Removing the
    calling processor causes undefined behavior or silent crash.

Source:
  ScriptingApiObjects.cpp  ScriptBuilder::clearChildren()
    -> dynamic_cast<Chain*> for target chain
    -> sendDeleteMessage() + handler remove per child
