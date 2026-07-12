Sets the sync mode for the global master clock, controlling how the internal clock (started/stopped by `startInternalClock`/`stopInternalClock`) interacts with the external DAW clock. Use the TransportHandler constants as the argument. This is a global setting - it affects all TransportHandler instances.

| Mode | Behaviour |
|------|----------|
| Inactive (0) | All clock processing disabled. No transport, beat, or grid callbacks fire. `getPPQPos()` returns 0. |
| ExternalOnly (1) | Only DAW transport drives callbacks. Internal clock calls are accepted but ignored (no internal playhead created). Grid timing follows DAW PPQ position. |
| InternalOnly (2) | Only script-driven clock drives callbacks. DAW transport is completely ignored. Grid and PPQ use internal uptime counter. |
| PreferInternal (3) | Internal clock takes priority. When internal is playing, external events are dropped. When internal is idle, external events drive transport normally. |
| PreferExternal (4) | DAW clock takes priority. When DAW is playing, internal start/stop events are dropped. When DAW starts, handoff from internal to external fires a grid resync. |
| SyncInternal (5) | Internal clock drives start/stop, but PPQ position continuously syncs to DAW position while DAW is playing. External stop commands are ignored - internal clock keeps running. |
