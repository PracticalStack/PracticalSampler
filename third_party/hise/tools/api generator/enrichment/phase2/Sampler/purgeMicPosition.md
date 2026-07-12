## purgeMicPosition

**Examples:**

```javascript:multi-sampler-mic-purge
// Title: Multi-sampler mic position management with mode matrix
// Context: A multi-mic instrument purges/unpurges mic positions across all samplers
// based on a mode matrix (e.g., Classic uses all mics, Pop uses only Mid).

const var NUM_MICS = 3;

const var samplers = [Synth.getSampler("Sustain"),
                      Synth.getSampler("Release"),
                      Synth.getSampler("Pedal")];

const var micGains = [Synth.getEffect("Close"),
                      Synth.getEffect("Mid"),
                      Synth.getEffect("Far")];

const var micNames = ["Close", "Mid", "Far"];

// Mode matrix: 1 = user-controlled, 0 = forced off, 2 = forced on
//                CLOSE  MID  FAR
const var MATRIX = [[1,    1,   1],   // Classic
                    [0,    2,   0],   // Pop
                    [0,    2,   2]];  // Cinematic

reg currentMode = 0;

inline function purgeMic(micIndex, shouldBeEnabled)
{
    // Purge the mic position across ALL samplers simultaneously
    for (s in samplers)
        s.purgeMicPosition(micNames[micIndex], !shouldBeEnabled);
}

inline function refreshAllMics()
{
    for (i = 0; i < NUM_MICS; i++)
    {
        local modeValue = MATRIX[currentMode][i];

        // 2 = forced on regardless of button, 0 = forced off, 1 = user choice
        if (modeValue == 2)
            purgeMic(i, true);
        else if (modeValue == 0)
            purgeMic(i, false);
        else
            purgeMic(i, micButtons[i].getValue());
    }
}
```

```json:testMetadata:multi-sampler-mic-purge
{
  "testable": false,
  "skipReason": "Requires multi-mic sampler module tree with loaded samples"
}
```

The mic name passed to `purgeMicPosition()` must exactly match the channel suffix configured in the sampler (e.g., `"Close"`, `"Mid"`, `"Far"`). Use `getMicPositionName()` to discover the correct names at runtime.

**Pitfalls:**
- When managing mic positions across multiple samplers, you must call `purgeMicPosition()` on each sampler individually. There is no global purge API - iterate the sampler array.
