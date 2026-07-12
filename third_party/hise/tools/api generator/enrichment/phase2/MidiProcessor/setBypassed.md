## setBypassed

**Examples:**

```javascript:multi-channel-mute-solo
// Title: Multi-channel mute/solo system using MidiMuter bypass
// Context: A multi-channel instrument stores an array of MidiMuter handles
// and toggles them by index. This is the most common MidiProcessor pattern.

const var NUM_CHANNELS = 4;
const var muters = [];

// Cache all processor references at init time
for (i = 0; i < NUM_CHANNELS; i++)
    muters.push(Synth.getMidiProcessor("MidiMuter" + (i + 1)));

// Mute state tracked in a parallel array
const var muteState = [];

for (i = 0; i < NUM_CHANNELS; i++)
    muteState.push({"Mute": false, "Solo": false});

inline function shouldBeActive(channelIndex)
{
    // Solo has highest priority
    if (muteState[channelIndex].Solo)
        return true;

    if (muteState[channelIndex].Mute)
        return false;

    // If any sibling is soloed, this channel is inactive
    for (s in muteState)
    {
        if (s.Solo)
            return false;
    }

    return true;
}

inline function refreshMuteState()
{
    for (i = 0; i < NUM_CHANNELS; i++)
    {
        // MidiMuter's attribute 0 is the mute toggle (1 = muted)
        muters[i].setAttribute(0, 1 - shouldBeActive(i));
    }
}
```
```json:testMetadata:multi-channel-mute-solo
{
  "testable": false,
  "skipReason": "Requires MidiMuter modules in the module tree; utility functions defined but not invoked"
}
```

```javascript:conditional-mode-bypass
// Title: Conditional MIDI processing bypass
// Context: Enable or disable MIDI processors based on a mode selection.
// setBypassed(true) stops the module from processing MIDI events entirely.

const var retriggerScript = Synth.getMidiProcessor("RetriggerScript");
const var velocityFilter = Synth.getMidiProcessor("VelocityFilter");
const var legatoFilter = Synth.getMidiProcessor("LegatoFilter");

inline function onModeChanged(component, value)
{
    local mode = parseInt(value);

    // Mode 0: all processing active
    // Mode 1: bypass velocity and legato filters
    retriggerScript.setBypassed(mode == 2);
    velocityFilter.setBypassed(mode != 0);
    legatoFilter.setBypassed(mode != 0);
}
```
```json:testMetadata:conditional-mode-bypass
{
  "testable": false,
  "skipReason": "Callback-driven pattern requiring named MIDI processor modules in the module tree"
}
```
