DspNetwork::undo() -> Integer

Thread safety: UNSAFE -- calls UndoManager::undo() which replays ValueTree property changes, potentially triggering listeners and allocations.
Undoes the last action in the network's undo history. Uses the internal JUCE
UndoManager which groups operations into transactions every 1500ms. Undo is enabled
by default in the HISE backend (IDE) but disabled in exported plugins. Returns true
if an action was successfully undone.
Anti-patterns:
  - Do NOT call in exported plugins -- [BUG] crashes with nullptr dereference.
    getUndoManager(true)->undo() is called without null-checking, and getUndoManager()
    returns nullptr when enableUndo is false (frontend default).
Source:
  DspNetwork.cpp  undo()
    -> getUndoManager(true)->undo()
    -> UndoManager groups transactions every 1500ms via timerCallback
    -> BUG: no null check on getUndoManager() return value in frontend builds
