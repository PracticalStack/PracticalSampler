Forces the next grid callback to have its `firstGridInPlayback` argument set to `true`, providing a manual resync point for the grid. Useful when you need to reset sequencer state to the beginning after loading a preset or switching patterns.

> [!Warning:Affects all TransportHandler instances] This is a global operation - it affects all TransportHandler instances, not just the one you call it on.
