# Phase 4b: LLM C++ Reference -- Agent Instructions

Phase 4b produces structured, concise reference entries for LLMs writing HISEScript or C++ `HardcodedScriptProcessor` code. There are two kinds of entries:

- **Method entries** (`methodName.md`) -- one per method, with signature, thread safety, dispatch mechanics, source locations, and anti-patterns.
- **Class entries** (`Readme.md`) -- one per class, with description, constants, complexity tiers, practical defaults, common mistakes, and a canonical usage example.

Both are standalone files optimized for machine consumption: dense, structured, and explicit.

**Audience:** LLM coding agents that need to make correct API choices when writing MIDI/DSP logic in HISEScript or C++ `HardcodedScriptProcessor` code. Not for human readers (that's Phase 4a).

**ASCII-only rule:** All output files must use ASCII characters only. Use `--` instead of em-dashes, straight quotes instead of curly quotes.

> **Note:** For Phase 4a (human-facing docs), see `phase4.md`.

---

## Key Principle: No New Exploration

Phase 4b is a **formatting pass over existing Phase 1 and Phase 2 data**. The agent reads existing enrichment artifacts and reformats them into output templates. It does NOT explore C++ source code, run searches, or derive new information.

If the exploration file is missing or insufficient for a given class, the correct response is to flag that class as needing a Phase 1 re-run -- not to explore from Phase 4b.

---

## Source Material

The agent reads from up to four files per class:

### 1. `enrichment/phase1/ClassName/methods.md` (method entries)

Provides per-method structured data:
- Signature, return type
- callScope and callScopeNote
- Description
- Parameters (name, type, forced, description, constraints)
- Callback Signature (for methods with `Function` parameters)
- Pitfalls
- Cross-references
- Examples

### 2. `enrichment/resources/explorations/ClassName.md` (method entries)

Provides raw C++ source extracts from Phase 1 exploration:
- File locations (header, implementation, related files) with line numbers
- Full class declaration
- Method implementations with source code
- Call chains showing dispatch from scripting API through to engine internals
- Internal data structures and threading patterns

### 3. `enrichment/phase1/ClassName/Readme.md` (method + class entries)

Provides class-level context:
- Brief and Purpose (class description)
- obtainedVia (how to get an instance)
- Constants and dynamic constants
- Common mistakes
- codeExample (canonical usage pattern)
- Alternatives
- Class-level architecture notes

### 4. `enrichment/phase2/ClassName/Readme.md` (class entries only)

Provides real-project analysis:
- Real-World Use Cases (actual plugin patterns observed in projects)
- Complexity Tiers (ordered from simplest to most complex usage, with method subsets)
- Practical Defaults (opinionated recommendations for parameter values and patterns)
- Integration Patterns (cross-class wiring patterns)
- Additional Common Mistakes from project observations

---

## Output

```
enrichment/phase4b/ClassName/Readme.md         # One class-level entry
enrichment/phase4b/ClassName/methodName.md     # One file per method
```

One Readme.md per class (the class-level card) plus one file per method. Method filenames must match the method name exactly (case-sensitive).

---

# Method Entries

## Method Entry Template

```
ClassName::methodName(params) -> returnType

Thread safety: SAFE|WARNING|UNSAFE|INIT + explanation
[1-2 line description]
Callback signature: f(Type1 param1, Type2 param2)
Required setup: [minimal code to call this method]
Dispatch/mechanics: [what happens internally -- 1-3 lines]
Pair with: [companion methods and why]
Anti-patterns: [what NOT to do and why]
Source:
  file.cpp:line  functionName() -> innerCall() -> deeperCall()
  [key code snippet if illuminating]
```

---

## Method Field-by-Field Instructions

### Header Line

```
ClassName::methodName(Type1 param1, Type2 param2) -> ReturnType
```

- Use the VarTypes from `methods.md` (e.g., `Double`, `String`, `Function`, `Number`)
- Use `-> undefined` for void methods
- Match the signature exactly as documented in `methods.md`

### Thread Safety

Map from `callScope` in `methods.md`:

| callScope | Phase 4b label | Notes |
|-----------|---------------|-------|
| `safe` | `SAFE` | No qualifiers needed unless callScopeNote exists |
| `warning` | `WARNING` | Always include the callScopeNote explanation |
| `unsafe` | `UNSAFE` | Note what makes it unsafe (allocations, locks, ValueTree mutation) |
| `init` | `INIT` | Note that runtime calls throw a script error |
| `null` | `UNKNOWN -- treat as UNSAFE` | Flag for Phase 1 re-investigation |

If `callScopeNote` exists in `methods.md`, append it after the label:
```
Thread safety: WARNING -- allocates in backend builds (USE_BACKEND), compiled out in exported plugins
```

### Description

1-2 lines. Pull from the `Description` field in `methods.md`. Compress to the essential behavior. Strip prose filler.

**Good:** `Converts input from local range to normalised 0..1 and sends to all cable targets.`
**Bad:** `This method takes a value and converts it using the configured range and then sends it to all targets that are registered with the cable system.`

### Callback Signature

For methods that accept a `Function` parameter, include the typed callback signature so the LLM knows exactly how to declare the callback function.

Pull from the `**Callback Signature:**` field in `methods.md`. Reformat to the compact Phase 4b style.

`methods.md` format: `**Callback Signature:** f(gridIndex: int, timestamp: int, firstGridInPlayback: bool)`
Phase 4b format: `Callback signature: f(int gridIndex, int timestamp, bool firstGridInPlayback)`

Skip this field entirely if:
- The method does not take a `Function` parameter
- No `Callback Signature` field exists in `methods.md`

### Required Setup

Show the minimal code to obtain the object and call the method. Skip this field entirely for:
- Namespace methods (e.g., `Math.max()`, `Engine.getHostBpm()`)
- Methods where the object token is obvious and no setup is needed

Include this field when:
- The object must be obtained via a factory method
- The method requires prior configuration (e.g., setting a range before calling `setValue`)

```
Required setup:
  const var cable = Engine.getGlobalRoutingManager().getCable("myCable");
  cable.setRange(0, 127);
```

### Dispatch/Mechanics

What happens inside, from the scripting API wrapper down to the engine. Pull from the exploration `.md` file which contains the actual call chains and source code.

Skip this field for:
- Simple getters that return a cached value
- Simple setters that assign a value
- Trivial wrappers

Include this field when:
- The method dispatches to multiple targets (e.g., `setValue` fans out to listeners)
- The method has non-obvious internal behavior (e.g., range conversion, clamping)
- The call chain reveals threading or synchronization patterns

Format:
```
Dispatch/mechanics:
  convertTo0to1(input) -> Cable::sendValue(normalised)
    -> iterates targets: GlobalCableNode, ScriptComponent, MacroControl
    -> each target applies its own output range conversion
```

Keep to 1-3 lines. Not a full source code dump -- just enough for an LLM to understand what happens.

### Pair With

Companion methods that are typically used together. Pull from `Cross References` in `methods.md`, but be selective -- only include methods that form a functional pair or workflow.

**Include:**
- `addNoteOn` pairs with `noteOffByEventId` (must release what you create)
- `setValue` pairs with `setRange` (range must be configured first)
- `registerCallback` pairs with `deregisterCallback` (lifecycle)

**Skip:**
- Loose associations (e.g., `getValue` cross-referencing `getValueNormalised` -- these are alternatives, not pairs)

Format:
```
Pair with:
  setRange/setRangeWithSkew/setRangeWithStep -- must configure range before setValue
  registerCallback -- to observe value changes from other sources
```

### Anti-Patterns

What NOT to do. Pull from:
- `Pitfalls` in `methods.md`
- `Common Mistakes` in `Readme.md`
- Insights from the arpeggiator decision tree exercise (documented in the master schema's strategic context)

Format:
```
Anti-patterns:
  - Do NOT call without setting a range first -- defaults to 0..1 identity, making setValue equivalent to setValueNormalised
  - Do NOT use Array to store note events -- Array.push() allocates on the audio thread
```

Skip this field if there are no meaningful anti-patterns for the method.

### Source

File path, line number, and call chain summary. Pull directly from the exploration `.md` file.

Format:
```
Source:
  ScriptingApiObjects.cpp:8952  GlobalCableReference::setValue()
    -> convertTo0to1(input) using NormalisableRange
    -> cable->sendValue(normalised, sendNotification)
```

Always include the file path and line number. The call chain is optional but strongly preferred for non-trivial methods.

---

# Class Entries

Each enriched class gets a `Readme.md` that serves as its overview card. The card gives an LLM everything it needs to understand what the class is, how to obtain an instance, which methods to use for a given complexity level, and what mistakes to avoid -- before drilling into individual methods.

## Class Entry Template

```
ClassName (category)
Obtain via: <obtainedVia expression>

<1-3 line description>

[Constants:
  GroupName:
    Name = value    description
    ...]

Complexity tiers:
  1. Tier name: method1, method2, method3. When to use this tier.
  2. ...

Practical defaults:
  - Opinionated recommendation with rationale.
  - ...

Common mistakes:
  - Description of mistake -- consequence.
  - ...

Example:
  <canonical usage code>

Methods (N):
  name1  name2  name3  ...
```

## Class Field-by-Field Instructions

### Header Line

```
ClassName (category)
```

The category is one of: `namespace` (global, e.g., `Synth`, `Console`), `object` (obtained via factory method, e.g., `GlobalCable`, `MidiPlayer`). Pull from Phase 1 Readme.md class metadata.

### Obtain Via

```
Obtain via: Engine.createTransportHandler()
```

The HISEScript expression to get an instance. Pull from Phase 1 `obtainedVia`. Omit this line entirely for global namespaces where the class name is the access path (e.g., `Console`, `Math`).

If Phase 1 has prose around the expression (e.g., "Created via `Engine.createMidiList()`. The factory method..."), extract just the code expression.

### Description

1-3 lines distilled from Phase 1 `Brief` and `Purpose`. Capture what the class does and its primary role. Strip internal architecture details -- those belong in Phase 4a.

**Good:** `Named data bus for routing normalised values and arbitrary data between script processors and modules.`
**Bad:** `GlobalCable is a class that wraps the Cable object in the GlobalRoutingManager and provides scripting API methods for reading and writing values through a named bus system.`

### Constants

Only include if the class has constants in Phase 1. Group by the `Group` field. Format:

```
Constants:
  SyncModes:
    Inactive = 0           No syncing going on
    ExternalOnly = 1       Only reacts on external clock events
```

Align values and descriptions for readability. Omit this section entirely if the class has no constants.

### Complexity Tiers

Pull from Phase 2 `### Complexity Tiers`. Distill each tier to one line: tier name, key methods, and when to use it. This tells the LLM "if you're doing X, you only need these methods."

```
Complexity tiers:
  1. Basic read/write: getValue, setValue, setValueNormalised. Simple inter-script communication.
  2. Callback-driven: + registerCallback with AsyncNotification. Reactive UI updates.
  3. Data channel: + sendData, registerDataCallback. Structured JSON through cables.
  4. Module integration: + connectToModuleParameter, connectToMacroControl. Direct module wiring.
```

Use `+` prefix to indicate methods added on top of the previous tier.

Omit this section entirely if Phase 2 has no Complexity Tiers (e.g., for simple classes like Console).

### Practical Defaults

Pull from Phase 2 `### Practical Defaults`. These are opinionated recommendations -- the most important section for preventing LLMs from making bad design choices.

```
Practical defaults:
  - Use AsyncNotification for value callbacks unless you need audio-thread timing.
  - Prefer timer-polled getValue() over SyncNotification callbacks for UI updates.
  - Use setValueNormalised for cable-to-cable routing (internal transport is 0..1).
```

Each item should be actionable and specific. Not "consider using X" but "use X because Y."

Omit this section entirely if Phase 2 has no Practical Defaults.

### Common Mistakes

Merge common mistakes from Phase 1 and Phase 2 Readme.md files. Deduplicate -- if both phases list the same mistake, include it once. Condense each to one line: what's wrong + consequence.

```
Common mistakes:
  - Registering a grid callback without calling setEnableGrid() first -- silently never fires.
  - Using a regular function with SyncNotification -- must use inline function (audio thread).
```

Order by severity (most likely to cause silent bugs first, then errors, then style issues).

### Example

Use the `codeExample` from Phase 1 Readme.md. This is the canonical usage pattern showing how the class is obtained, configured, and used end-to-end. The agent may lightly edit for clarity (fix formatting, remove redundant comments) but must NOT invent new code.

```
Example:
  const var th = Engine.createTransportHandler();
  th.setOnTempoChange(SyncNotification, onTempoChange);
  th.setEnableGrid(true, 7);
  ...
```

Indent all code lines by 2 spaces. If the Phase 1 codeExample is missing, omit this section -- do NOT invent an example.

### Methods

Sorted alphabetically. Just names, no signatures. The LLM queries individual methods via `query_scripting_api("ClassName.method")` for full details.

```
Methods (19):
  getGridLengthInSamples      getGridPosition
  isNonRealtime               isPlaying
  ...
```

Multi-column layout for compactness (2-3 names per line, aligned). The count in parentheses must match the actual number of methods in `methods.md`.

---

## Workflow

### Per-class execution

1. Read `enrichment/phase1/ClassName/Readme.md` -- class context, constants, codeExample
2. Read `enrichment/phase2/ClassName/Readme.md` -- complexity tiers, practical defaults
3. Read `enrichment/phase1/ClassName/methods.md` -- all method data
4. Read `enrichment/resources/explorations/ClassName.md` -- source code extracts
5. Write class-level entry:
   a. Distill class description, constants, complexity tiers, practical defaults, common mistakes, and codeExample into the class entry template
   b. Write to `enrichment/phase4b/ClassName/Readme.md`
6. For each method in `methods.md`:
   a. Extract the relevant data from sources 1, 3, and 4
   b. Format into the method entry template
   c. Write to `enrichment/phase4b/ClassName/methodName.md`
7. Verify: Readme.md exists, all methods have an output file

### Session prompt

```
Follow tools/api generator/doc_builders/scripting-api-enrichment/phase4b.md.
Run Phase 4b authoring for [ClassName].
```

### Batch mode

Phase 4b can run on all enriched classes in a single session since it's a pure formatting pass. The agent iterates over all classes that have both a `methods.md` and an exploration file.

---

## What Phase 4b Does NOT Do

- Does NOT explore C++ source code (already done in Phase 1)
- Does NOT produce human-friendly prose (that's Phase 4a)
- Does NOT write into `api_reference.json` -- Phase 4b files are standalone
- Does NOT render diagrams (that's Phase 4a)
- Does NOT invent new examples -- the class entry uses the existing codeExample from Phase 1
- Does NOT infer new information -- everything comes from Phase 1 and Phase 2 artifacts
- Does NOT produce C++ syntax examples (deferred -- see below)

### Future: Dual-Syntax Documentation (cppExample field)

The HISE scripting API is shared between HISEScript and C++ via
`HardcodedScriptProcessor` (and in the future, the DLL-hotswappable MIDI
processor pipeline). Post-MVP, Phase 4b entries will be extended with a
`cppExample` field showing the C++ syntax for the same API call. This is
tracked in [hise_api_generator#13](https://github.com/christoph-hart/hise_api_generator/issues/13)
and depends on the proxy class design (issue #7) being finalized.

**Do NOT invent a `cppExample` format until then.** The current entries use
HISEScript syntax only, which is correct for now. The C++ syntax pass will be
an additive update to existing entries, not a rewrite.

---

## Quality Checklist

### Method entries

- [ ] Header line matches the signature in `methods.md` exactly
- [ ] Thread safety label matches `callScope` from `methods.md`
- [ ] Description is 1-2 lines, not prose paragraphs
- [ ] Callback signature present for methods with `Function` parameters (from `methods.md`)
- [ ] Source file path and line number are present
- [ ] Anti-patterns (if any) describe concrete consequences, not vague warnings
- [ ] No em-dashes, curly quotes, or non-ASCII characters
- [ ] All fields are sourced from Phase 1 artifacts -- nothing invented

### Class entries

- [ ] Header line has correct class name and category
- [ ] `Obtain via:` matches Phase 1 `obtainedVia` (or omitted for global namespaces)
- [ ] Description is 1-3 lines, distilled from Phase 1 Brief + Purpose
- [ ] Constants match Phase 1 data exactly (values, groups, descriptions)
- [ ] Complexity tiers sourced from Phase 2 (omit section if Phase 2 has none)
- [ ] Practical defaults sourced from Phase 2 (omit section if Phase 2 has none)
- [ ] Common mistakes deduplicated across Phase 1 + Phase 2 (no duplicates)
- [ ] Example is the Phase 1 `codeExample` (may be lightly edited for clarity, not invented)
- [ ] Method count matches the number of methods in `methods.md`
- [ ] Method names sorted alphabetically
- [ ] No em-dashes, curly quotes, or non-ASCII characters

---

## Method Reference Examples

### GlobalCable::setValue(Double inputWithinRange) -> undefined

```
GlobalCable::setValue(Double inputWithinRange) -> undefined

Thread safety: SAFE
Converts input from local range to normalised 0..1 and sends to all cable targets.
Clamped internally. Without a configured range, behaves identically to setValueNormalised().

Required setup:
  const var cable = Engine.getGlobalRoutingManager().getCable("myCable");
  cable.setRange(0.0, 127.0);

Dispatch/mechanics:
  convertTo0to1(input) via NormalisableRange -> Cable::sendValue(normalised)
    -> fans out to: GlobalCableNode (scriptnode), ScriptComponent, MacroControl, OSC target
    -> each target applies its own output range

Pair with:
  setRange/setRangeWithSkew/setRangeWithStep -- configure range before calling setValue
  getValue -- reads back the value in the local range
  registerCallback -- observe value changes from other sources

Anti-patterns:
  - Do NOT call without setting a range first -- defaults to 0..1 identity, making
    setValue and setValueNormalised equivalent (confusing, likely a bug)
  - Do NOT use sendData() for numeric values -- sendData is for JSON/string/buffer payloads,
    setValue is for normalised numeric dispatch

Source:
  ScriptingApiObjects.cpp:8952  GlobalCableReference::setValue()
    -> convertTo0to1(input) using NormalisableRange
    -> cable->sendValue(normalised, sendNotification)
    -> iterates registeredTargets: Cable::Target::sendValue()
```

### TransportHandler::setOnGridChange(Number sync, Function f) -> undefined

Example with callback signature field:

```
TransportHandler::setOnGridChange(Number sync, Function f) -> undefined

Thread safety: UNSAFE -- allocates new Callback, registers MusicalUpdateListener
Registers callback for high-precision grid events.
Grid must be enabled via setEnableGrid() first. Does NOT fire immediately on registration.
Callback signature: f(int gridIndex, int timestamp, bool firstGridInPlayback)
Required setup:
  const var th = Engine.createTransportHandler();
  th.setEnableGrid(true, 11); // 1/16 note
Dispatch/mechanics:
  Same MusicalUpdateListener pattern as setOnBeatChange
  Grid index adjusted by localGridMultiplier (bit shift + mask filtering)
  timestamp = sample offset within audio block for sample-accurate scheduling
Pair with:
  setEnableGrid -- must enable grid first (silent no-op otherwise)
  setLocalGridMultiplier -- per-instance rate division
  getGridLengthInSamples -- compute grid duration
Anti-patterns:
  - Do NOT register grid callback without calling setEnableGrid first -- silently never fires
Source:
  ScriptingApi.cpp:8596  setOnGridChange() -> addMusicalUpdateListener() + new Callback("onGridChange", f, isSync, 3)
```

---

## Class Reference Example

### TransportHandler

```
TransportHandler (namespace)
Obtain via: Engine.createTransportHandler()

Registers callbacks for DAW transport events: tempo, playback, beats, grid,
time signature, and bypass. Central timing authority for plugins with internal
sequencers or clock-synced behavior.

Constants:
  SyncModes:
    Inactive = 0           No syncing going on
    ExternalOnly = 1       Only reacts on external clock events
    InternalOnly = 2       Only reacts on internal clock events
    PreferInternal = 3     Override with internal clock when it is playing
    PreferExternal = 4     Override with external clock when it is playing
    SyncInternal = 5       Sync internal clock when external playback starts

Complexity tiers:
  1. DAW-only transport: setOnTransportChange, setOnTempoChange. React to host
     transport without an internal clock. Default sync mode works.
  2. Internal clock with host fallback: + setSyncMode(PreferInternal),
     startInternalClock, stopInternalClock, stopInternalClockOnExternalStop.
     Plugins with their own play/stop controls.
  3. Full transport system: + setEnableGrid, setOnGridChange,
     sendGridSyncOnNextCallback, setLocalGridMultiplier. MIDI-triggered clock
     start with sample-accurate timestamps, host-sync toggle, grid timing.

Practical defaults:
  - Use PreferInternal as the default sync mode for plugins with their own
    transport controls. Switch to PreferExternal only when the user enables
    host sync.
  - Tempo factor 8 (1/8 note) is a good grid resolution for drum sequencers --
    fast enough for detailed patterns, slow enough for manageable callback
    frequency.
  - Always enable stopInternalClockOnExternalStop(true) when using
    PreferInternal with a host-sync option.
  - Use AsyncNotification for the transport change callback when it updates UI.
    Bridge to a Broadcaster for multi-file state propagation.

Common mistakes:
  - Registering a grid callback without calling setEnableGrid() first --
    silently never fires, no error reported.
  - Using a regular function with SyncNotification -- must use inline function
    for synchronous callbacks (audio thread).
  - Calling startInternalClock(0) from a MIDI callback -- use
    startInternalClock(Message.getTimestamp()) for sample-accurate timing.
  - Not stopping the internal clock before loading a preset -- causes timing
    discontinuities. Stop first, sendGridSyncOnNextCallback, then restart.

Example:
  const var th = Engine.createTransportHandler();

  // Synchronous tempo callback (audio thread, requires inline function)
  inline function onTempoChange(newTempo)
  {
      // React to tempo changes on the audio thread
  }

  th.setOnTempoChange(SyncNotification, onTempoChange);

  // Asynchronous transport callback (UI thread, any function works)
  inline function onTransportChange(isPlaying)
  {
      Console.print(isPlaying ? "Playing" : "Stopped");
  }

  th.setOnTransportChange(AsyncNotification, onTransportChange);

  // High-precision grid for sample-accurate sequencing
  th.setEnableGrid(true, 7); // 1/8 note grid

  inline function onGridChange(gridIndex, timestamp, firstGrid)
  {
      if (firstGrid)
          Console.print("Grid restarted");
  }

  th.setOnGridChange(SyncNotification, onGridChange);

  // Set sync mode for internal/external clock interaction
  th.setSyncMode(th.PreferExternal);

Methods (19):
  getGridLengthInSamples      getGridPosition
  isNonRealtime               isPlaying
  sendGridSyncOnNextCallback  setEnableGrid
  setLinkBpmToSyncMode        setLocalGridBypassed
  setLocalGridMultiplier      setOnBeatChange
  setOnBypass                 setOnGridChange
  setOnSignatureChange        setOnTempoChange
  setOnTransportChange        setSyncMode
  startInternalClock          stopInternalClock
  stopInternalClockOnExternalStop
```
