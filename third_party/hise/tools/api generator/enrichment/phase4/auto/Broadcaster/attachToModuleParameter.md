Registers the broadcaster as a source that fires whenever a module parameter changes on the specified processor(s). Pass module IDs as strings (not object references) and parameter identifiers as strings or integers.

Three special parameter IDs are supported:

| ID | Behaviour |
|---|---|
| `"Bypassed"` | Fires with `1.0` when bypassed, `0.0` when active |
| `"Enabled"` | Fires with `1.0` when enabled, `0.0` when bypassed (inverse of `"Bypassed"`) |
| `"Intensity"` | Monitors a Modulator's intensity value (only valid on Modulator-type processors) |

On attachment, existing listeners immediately receive the current value of each watched parameter. Queue mode is automatically enabled.

> [!Warning:Pass module IDs as strings] Pass module IDs as strings, not scripting references from `Synth.getEffect()` or similar. Passing an object reference produces an error.
