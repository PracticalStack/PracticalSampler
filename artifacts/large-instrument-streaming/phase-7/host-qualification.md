# Accurate Salamander host qualification

Result: PASS

Configuration: Release x64, normal 1 MiB process stack, Windows 11 Home, MSVC 19.44.

Input: the 2,631,961,513-byte package produced by the real-corpus qualification run. The package is a temporary licensed-data-derived artifact and is removed after qualification.

Final host metrics:

| Metric | Standalone | Plug-in |
|---|---:|---:|
| Open/prepare | 1,869 ms / 1,869,006 us | 917 ms / 916,623 us |
| Activation | 289 us | 768 us |
| Engine activation | 206 us | 559 us |
| Workspace transition | 20 us | 37 us |
| Sustained playback exercise | 5,625 ms | 5,794 ms |
| Cache misses/page intents | 2,672 | 2,672 |
| Background page reads | 1,140 | 1,096 |
| Realtime violations | 0 | 0 |

Host-state restore completed in 1,904 ms. `setStateInformation()` itself returned in 54 us and queued package locator preparation off-thread. Restored playback was audible with the editor closed, and an editor opened successfully afterward.

The nonzero background-read counts prove that the real v2 package serviced pages beyond its resident heads. Neither shell performed package I/O on the audio callback, and both used the same deferred session/readiness path.
