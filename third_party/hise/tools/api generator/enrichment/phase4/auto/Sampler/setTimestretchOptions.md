Configures the timestretch engine from a JSON object. The `Mode` property selects one of four modes:

| Mode | Behaviour |
|------|-----------|
| `"Disabled"` | No timestretching applied |
| `"VoiceStart"` | New voices use the current ratio; active voices keep theirs |
| `"TimeVariant"` | All active voices update to the same ratio in real-time |
| `"TempoSynced"` | Ratio calculated automatically from sample length and current tempo |

Additional properties: `Tonality` (0.0-1.0), `SkipLatency` (boolean), `NumQuarters` (for TempoSynced mode), and `PreferredEngine` (engine identifier). Use `Sampler.getTimestretchOptions()` to read the current configuration.
