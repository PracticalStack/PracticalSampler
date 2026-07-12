# Tempo and Transport Infrastructure Reference

Distilled from C++ source for the scriptnode node enrichment pipeline.
All information derived exclusively from HISE source code.

Source files consulted:
- `hi_tools/hi_tools/MiscToolClasses.h` (TempoSyncer, MasterClock, TempoListener)
- `hi_tools/hi_tools/MiscToolClasses.cpp` (TempoSyncer implementation, tempo factor table)
- `hi_dsp_library/snex_basics/snex_Types.h` (DllBoundaryTempoSyncer, PolyHandler)
- `hi_dsp_library/dsp_nodes/CoreNodes.h` (clock_ramp implementation)
- `hi_dsp_library/dsp_nodes/CableNodes.h` (tempo_sync, ppq, transport implementations)

---

## 1. TempoSyncer: Tempo Values and Conversion

`TempoSyncer` is a static utility class that defines musical tempo values and
converts between tempo enums, milliseconds, samples, and hertz.

### Tempo Enum

Each enum value has a string name and a "tempo factor" -- a multiplier relative
to a quarter note (factor 1.0). The conversion formula uses this factor:

```
duration_seconds = (60.0 / bpm) * tempoFactor
```

Standard tempo values (without HISE_USE_EXTENDED_TEMPO_VALUES):

| Enum Name | String | Factor | Relative to Quarter |
|---|---|---|---|
| Whole | "1/1" | 4.0 | 4x longer |
| HalfDuet | "1/2D" | 3.0 | dotted half (2.0 * 1.5) |
| Half | "1/2" | 2.0 | 2x longer |
| HalfTriplet | "1/2T" | 1.333 | triplet half (4.0 / 3.0) |
| QuarterDuet | "1/4D" | 1.5 | dotted quarter (1.0 * 1.5) |
| Quarter | "1/4" | 1.0 | reference value |
| QuarterTriplet | "1/4T" | 0.667 | triplet quarter (2.0 / 3.0) |
| EighthDuet | "1/8D" | 0.75 | dotted eighth (0.5 * 1.5) |
| Eighth | "1/8" | 0.5 | half a quarter |
| EighthTriplet | "1/8T" | 0.333 | triplet eighth (1.0 / 3.0) |
| SixteenthDuet | "1/16D" | 0.375 | dotted sixteenth (0.25 * 1.5) |
| Sixteenth | "1/16" | 0.25 | quarter of a quarter |
| SixteenthTriplet | "1/16T" | 0.167 | triplet sixteenth (0.5 / 3.0) |
| ThirtyTwoDuet | "1/32D" | 0.1875 | dotted 32nd (0.125 * 1.5) |
| ThirtyTwo | "1/32" | 0.125 | 1/8 of a quarter |
| ThirtyTwoTriplet | "1/32T" | 0.0833 | triplet 32nd (0.25 / 3.0) |
| SixtyForthDuet | "1/64D" | 0.09375 | dotted 64th (0.0625 * 1.5) |
| SixtyForth | "1/64" | 0.0625 | 1/16 of a quarter |
| SixtyForthTriplet | "1/64T" | 0.04167 | triplet 64th (0.125 / 3.0) |

Extended tempo values (HISE_USE_EXTENDED_TEMPO_VALUES only):

| Enum Name | String | Factor |
|---|---|---|
| EightBar | "8/1" | 32.0 |
| SixBar | "6/1" | 24.0 |
| FourBar | "4/1" | 16.0 |
| ThreeBar | "3/1" | 12.0 |
| TwoBars | "2/1" | 8.0 |

Note: The "Duet" suffix means "dotted" (factor * 1.5). The "Triplet" suffix
means triplet timing (divides the next longer value by 3 instead of 2).

### Naming Convention

The doxygen comments call dotted notes "duole" (e.g., "1/2D" is called
"a half note duole"). This is the German musical term. In English
documentation, use "dotted" instead. The string names use "D" suffix for
dotted and "T" suffix for triplet.

### Conversion Functions

All static methods. All treat bpm=0 as bpm=120 (fallback).

**getTempoFactor(Tempo t) -> float**
Returns the raw tempo factor for a given enum value. Out-of-range values
return the Quarter factor (1.0) as fallback.

**getTempoInSamples(bpm, sampleRate, Tempo t) -> double**
```cpp
seconds = (60.0 / bpm) * tempoFactor
samples = seconds * sampleRate
```
Also has an overload taking a raw float factor instead of an enum.

**getTempoInMilliSeconds(bpm, Tempo t) -> float**
```cpp
seconds = (60.0f / bpm) * tempoFactor
return seconds * 1000.0f
```

**getTempoInHertz(bpm, Tempo t) -> float**
```cpp
seconds = (60.0f / bpm) * tempoFactor
return 1.0f / seconds
```

**getTempoIndexForTime(bpm, milliSeconds) -> Tempo**
Finds the closest tempo enum value for a given duration in ms. Linear scan
through all tempo values, returns the one with smallest absolute difference.
Not fast, but useful for display/conversion.

**getTempoIndex(String name) -> Tempo**
Looks up a tempo enum by its string name (e.g., "1/4T"). Returns Quarter
as fallback on mismatch (with jassertfalse in debug).

**getTempoNames() -> StringArray**
Returns all tempo names as a string array. Used by nodes to populate
parameter value name lists.

**initTempoData()**
Must be called at application start to populate the internal name/factor
arrays. Without this, all conversions return garbage.

---

## 2. TempoListener: Receiving Tempo and Transport Events

`TempoListener` is an abstract base class that nodes implement to receive
tempo, transport, and position callbacks.

### Callback Methods (all virtual, all have empty default implementations)

**tempoChanged(double newTempo)**
Called when BPM changes. Called synchronously in the audio callback before
processing. Called once per block (not sample-accurate). The `newTempo`
value is in beats per minute.

**onTransportChange(bool isPlaying, double ppqPosition)**
Called when the DAW transport starts or stops. `ppqPosition` is the
playback position in quarter notes (PPQ = Pulses Per Quarter note, but
here used as a fractional quarter-note position).

**onResync(double ppqPosition)**
Called when the transport position needs to be resynced -- e.g., when
the DAW playback position jumps (user clicks a new position) or wraps
around a loop boundary. Provides the new PPQ position.

**onBeatChange(int beatIndex, bool isNewBar)**
Called on each musical beat, respecting the time signature denominator.
For 6/8 time, this fires twice as often as for 3/4 time.
Disabled by default -- requires addPulseListener() registration.

**onGridChange(int gridIndex, uint16 timestamp, bool firstGridEventInPlayback)**
Called on every grid change for sample-accurate sequencer implementations.
The `timestamp` provides the sample offset within the current block.
Disabled by default -- requires addPulseListener() registration.

**onSignatureChange(int newNominator, int numDenominator)**
Called when the time signature changes.

### Weak Reference Support

TempoListener uses JUCE_DECLARE_WEAK_REFERENCEABLE, allowing safe storage
via WeakReference. The DllBoundaryTempoSyncer stores listeners as weak
references in an UnorderedStack.

---

## 3. DllBoundaryTempoSyncer: The Tempo Distribution Hub

`DllBoundaryTempoSyncer` inherits from `TempoListener` and acts as a
broadcaster -- it receives tempo/transport events from the host and
distributes them to all registered node listeners.

### Access Pattern

Nodes access the syncer through the PolyHandler in PrepareSpecs:

```cpp
void prepare(PrepareSpecs ps)
{
    syncer = ps.voiceIndex->getTempoSyncer();
    syncer->registerItem(this);
}
```

The `PolyHandler` holds a `DllBoundaryTempoSyncer*` pointer, set via
`setTempoSyncer()` and accessed via `getTempoSyncer()`.

### Registration Behavior

When `registerItem(listener)` is called, it immediately fires:
1. `listener->tempoChanged(bpm)` -- with current BPM
2. `listener->onTransportChange(isPlaying, ppqPosition)` -- with current state

This ensures newly registered listeners get the current state without
waiting for the next change event. This is important: nodes that register
during prepare() immediately know the current BPM.

Deregistration happens in the node destructor:
```cpp
~my_node() {
    if (syncer != nullptr)
        syncer->deregisterItem(this);
}
```

### Listener Storage

Uses `UnorderedStack<WeakReference<TempoListener>, 256>` -- max 256
simultaneous listeners. Protected by a `SimpleReadWriteLock`:
- Write lock for register/deregister
- Read lock for broadcasting

### Cached State

The syncer maintains cached state that nodes can query directly:
- `bpm` (double, default 120.0) -- current tempo
- `isPlaying` (bool, default false) -- transport state
- `ppqPosition` (double, default 0.0) -- last known PPQ position

### PPQ Position Access

**getCurrentPPQPosition(int timestamp) -> double**

Returns the PPQ position at a given sample offset. If a `ppqFunction`
(std::function<double(int)>) is set, it calls that function for
sample-accurate position lookup. Otherwise returns the cached
`ppqPosition` (block-accurate only).

This is used by clock_ramp in handleHiseEvent to get the PPQ at the
exact timestamp of a note-on event.

### Additional Fields (non-tempo)

DllBoundaryTempoSyncer also serves as a general-purpose bridge for
cross-DLL-boundary data. These are unrelated to tempo but colocated:
- `publicModValue` -- ModValue pointer for public_mod nodes
- `additionalEventStorage` -- for extra MIDI event data
- `uuidManager` / `uuidRequestFunction` -- UUID management

---

## 4. MasterClock: Internal/External Clock Coordination

The `MasterClock` class manages the relationship between the DAW's
external transport and HISE's internal clock. It is upstream of the
tempo listener system -- it decides which clock source to use and feeds
the result to the TempoListener infrastructure.

### States

```
Idle                -- no clock running
InternalClockPlay   -- internal clock is active
ExternalClockPlay   -- external (DAW) clock is active
```

### Sync Modes

| Mode | Behavior |
|---|---|
| Inactive | No syncing |
| ExternalOnly | Only reacts to external (DAW) clock |
| InternalOnly | Only reacts to internal clock |
| PreferInternal | Uses internal clock when it plays, external otherwise |
| PreferExternal | Uses external clock when it plays, internal otherwise |
| SyncInternal | Syncs internal clock start to external playback start |

### Clock Grid

The MasterClock can subdivide time into a grid based on a TempoSyncer::Tempo
value. When enabled:
- `gridDelta` is calculated from the tempo factor and sample rate
- `processAndCheckGrid()` returns `GridInfo` with:
  - `change` -- true if a grid boundary was crossed this block
  - `firstGridInPlayback` -- true for the very first grid after play starts
  - `timestamp` -- sample offset of the grid boundary within the block
  - `gridIndex` -- sequential grid counter

### PPQ Position

`getPPQPos(int timestampFromNow)` computes the current PPQ position from
uptime, sample rate, and BPM:
```cpp
quarterMs = TempoSyncer::getTempoInMilliSeconds(bpm, TempoSyncer::Quarter)
```
(Exact implementation uses sample-based calculation.)

### Clock Tolerance

`setClockTolerance(double)` handles a Logic Pro-specific issue where
latency-compensated tracks need tolerance to correctly detect beat 1.

---

## 5. How Tempo-Related Nodes Use This Infrastructure

### Pattern: Register as TempoListener in prepare()

All tempo-aware nodes follow the same pattern:

```cpp
void prepare(PrepareSpecs ps)
{
    if (syncer == nullptr)  // register only once
    {
        syncer = ps.voiceIndex->getTempoSyncer();
        syncer->registerItem(this);
    }
}

~MyNode()
{
    if (syncer != nullptr)
        syncer->deregisterItem(this);
}
```

The "register only once" guard prevents re-registration when prepare()
is called multiple times (e.g., sample rate changes). The initial
registration fires tempoChanged() and onTransportChange() immediately.

### clock_ramp: BPM-to-Ramp Conversion

`clock_ramp` converts tempo into a 0..1 ramp signal synchronized to the
HISE clock. Key internals:

**State struct** holds per-voice ramp state:
- `deltaPerSample` -- computed from quarter note duration in samples:
  `deltaPerSample = 1.0 / quarterInSamples`
- `factor` -- computed from the selected tempo and multiplier:
  `factor = 1.0 / (tempoFactor * multiplier)`
- `offset` -- PPQ position at last transport start or resync
- `uptime` -- accumulated sample time since last reset
- Two update modes:
  - Continuous: `fmod((uptime + offset*factor) * deltaPerSample * factor, 1.0)`
  - Synced: `fmod((offset + uptime*deltaPerSample) * factor, 1.0)`

**Dirty value pattern:** When bpm or sampleRate changes (from tempoChanged
callback, which runs in the audio thread), the new values are stored in a
`dirtyValues` array and the actual recalculation happens in `recalcIfDirty()`
at the start of the next `tick()`. This avoids recalculating mid-block.

**InactiveMode:** When transport is not playing, the ramp outputs:
- LastValue (0) -- the last computed ramp value (frozen)
- Zero (1) -- always 0.0
- One (2) -- always 1.0

### tempo_sync: Tempo-to-Milliseconds Output

`tempo_sync` converts the current tempo to a duration in milliseconds and
sends it as a modulation signal. This is a control node (OutsideSignalPath)
with no audio processing.

Key behavior:
- Outputs `TempoSyncer::getTempoInMilliSeconds(bpm, currentTempo) * multiplier`
  when Enabled=true
- Outputs `unsyncedTime` (raw ms value) when Enabled=false
- The modulation output uses `no_mod_normalisation` -- values are raw
  milliseconds, not 0..1 normalized
- Only sends a modulation update when the value actually changes
  (handleModulation checks `lastMs != currentTempoMilliseconds`)

### ppq: Transport Position Output

`ppq` outputs the current transport position as a normalized 0..1 value
within a configurable loop length. This is a control node.

Key behavior:
- `loopLengthQuarters = tempoFactor * multiplier` defines the cycle length
  in quarter notes
- Output: `fmod(ppqPos, loopLengthQuarters) / loopLengthQuarters`
- Only updates on transport start (`onTransportChange` with isPlaying=true)
  and resync events (`onResync`)
- Does NOT continuously advance -- it captures the PPQ at event time and
  wraps it into the loop range

### transport: Play State Output

`transport` is the simplest tempo-aware node. It outputs a boolean (0/1)
modulation signal reflecting the current transport play state.

### clock_base: Shared Base for Control Nodes

`tempo_sync`, `ppq`, and `transport` all inherit from `clock_base` which
handles the TempoListener registration pattern and marks nodes as
`OutsideSignalPath`. It also defines empty implementations for all
audio processing callbacks (SN_EMPTY_PROCESS, etc.).

---

## 6. PPQ Coordinate System

PPQ (Pulses Per Quarter note) is the standard musical time coordinate.
In HISE's implementation:

- PPQ position 0.0 = the start of playback
- PPQ position 1.0 = one quarter note later
- PPQ position 4.0 = one whole note (bar in 4/4 time)
- The position is a floating-point value, so fractional positions represent
  positions between beats

The ppqPosition received in callbacks comes from the DAW's
`AudioPlayHead::CurrentPositionInfo`. It advances continuously during
playback and can jump on loop boundaries or user seeks (triggering
`onResync`).

For clock_ramp, the PPQ position is used as a phase offset to keep the
ramp synchronized with the musical timeline rather than just the elapsed
sample count.

---

## 7. Thread Safety Considerations

- All TempoListener callbacks fire synchronously in the audio thread,
  before audio processing for the current block
- tempoChanged() fires only when BPM actually changes (checked in
  DllBoundaryTempoSyncer)
- onTransportChange() fires only when play state or position changes
- The dirty value pattern in clock_ramp::State defers recalculation to
  the processing path, avoiding mid-callback computation
- DllBoundaryTempoSyncer uses SimpleReadWriteLock for listener list access
- Listener storage uses WeakReference for safe deregistration even if the
  listener is destroyed without explicit deregisterItem()
