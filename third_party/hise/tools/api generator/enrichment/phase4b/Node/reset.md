Node::reset() -> undefined

Thread safety: SAFE -- pure virtual method designed for audio-thread invocation. Implementations must be lock-free.
Resets the node's internal DSP state (clears buffers, resets filters). Pure virtual --
each concrete node type implements its own reset logic. Called automatically at voice
start in polyphonic contexts.
Source:
  NodeBase.h  NodeBase::reset() = 0 (pure virtual)
    -> concrete implementations in each node type
