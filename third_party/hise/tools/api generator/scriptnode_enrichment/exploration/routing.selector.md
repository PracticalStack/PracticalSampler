# routing.selector - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/RoutingNodes.h:925`
**Base class:** `selector_base` (inherits `mothernode`) + `polyphonic_base`
**Classification:** audio_processor

## Signal Path

The selector dynamically routes channels within the multichannel buffer. The core logic is in the `op()` template method (line 977-1015). The behaviour depends on the `selectOutput` flag:

**SelectOutput = false (default, "select input"):** Copies data from the selected channel(s) at `ChannelIndex` to the front of the buffer (channels 0..NumChannels-1). If `ChannelIndex != 0`, it copies `data[ChannelIndex + i]` to `data[i]` for `i` in `0..numToProcess`. This moves a selected channel range to the front.

**SelectOutput = true ("select output"):** Copies data from the front of the buffer (channels 0..NumChannels-1) to the selected channel position. It copies `data[i]` to `data[ChannelIndex + i]`.

After the copy, if `ClearOtherChannels` is enabled:
- SelectOutput=false: channels from `numChannels` to end are zeroed (line 1011-1012)
- SelectOutput=true: all channels outside the range `[ChannelIndex, ChannelIndex+NumChannels)` are zeroed (lines 999-1007)

If `ChannelIndex == 0`, the copy step is skipped entirely (line 983: `if(c != 0)`), and only the clearing logic runs.

## Gap Answers

### select-output-meaning: What do the two SelectOutput values mean?

SelectOutput is a boolean toggle with text labels "Disabled" (0) and "Enabled" (1):
- **Disabled (0):** Selects input -- copies from channel `ChannelIndex` to channel 0 (brings a selected channel to the front).
- **Enabled (1):** Selects output -- copies from channel 0 to channel `ChannelIndex` (sends the first channel to a selected position).

### clear-other-channels-behaviour: When ClearOtherChannels is enabled, are non-selected channels zeroed? When disabled, do non-selected channels pass through unchanged?

Correct. When `ClearOtherChannels` is true (default), non-selected channels are set to zero via `data[i] = 0.0f`. When false, the clearing loop is skipped entirely, so non-selected channels retain whatever data they had (either original or from the copy operation).

### channel-routing-logic: How does the selector route channels?

With SelectOutput=false, ChannelIndex=2, NumChannels=2: the node copies channels 2,3 to channels 0,1. Then if ClearOtherChannels is on, channels 2 and above are zeroed. The description "router of the first channel (pair)" is accurate -- it moves selected channels to the first position(s).

The number of channels actually processed is `numToProcess = min(NumChannels, size - ChannelIndex)` (line 981), so it safely handles cases where the requested range exceeds available channels.

### polyphonic-channel-selection: Can different voices select different channels via modulated ChannelIndex?

Yes. `channelIndex` is stored as `PolyData<int, NV>` (line 1080). In `setChannelIndex()`, the value is set per-voice via `for(auto& s: channelIndex)` which iterates the current voice or all voices depending on context. During processing, `channelIndex.get()` returns the current voice's value. So per-voice channel selection is fully supported.

## Parameters

- **ChannelIndex** (0 -- 16, step 1, default 0): Which channel to select from (or route to, if SelectOutput is on).
- **NumChannels** (1 -- 16, step 1, default 1): How many consecutive channels to select.
- **SelectOutput** (0/1, default 0): Disabled = copy selected channels to front; Enabled = copy front channels to selected position.
- **ClearOtherChannels** (0/1, default 1): Whether to zero all channels outside the selected range.

## Polyphonic Behaviour

`PolyData<int, NV> channelIndex` allows per-voice channel selection. The other parameters (`numChannels`, `clearOtherChannels`, `selectOutput`) are stored as plain members in `selector_base` and are shared across all voices.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

## Notes

The `createParameters()` method uses `DEFINE_PARAMETERDATA(receive, ...)` for all parameters (lines 1051-1076), which means the parameter data structs reference the `receive` class name rather than `selector`. This is likely a copy-paste artifact but does not affect runtime behaviour since the parameter IDs come from the enum names, not the class name. The SelectOutput parameter range is set to `{1.0, 16.0, 1.0}` (line 1066) which does not match the actual boolean usage (0/1) -- this range is overridden by `setParameterValueNames` which provides "Disabled"/"Enabled" labels.
