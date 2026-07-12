Sends a parameter gesture begin or end message to the DAW host for a specific plugin parameter. Most DAWs only record automation changes that occur between a gesture begin and gesture end pair, so wrap any programmatic parameter changes (e.g. from a custom XY pad or panel control) with matching gesture calls.

The `automationType` argument identifies the parameter category:

| Value | Type |
|-------|------|
| 0 | Macro |
| 1 | Custom automation |
| 2 | Script UI control |
| 3 | NKS integration |

Returns true if a matching parameter was found.
