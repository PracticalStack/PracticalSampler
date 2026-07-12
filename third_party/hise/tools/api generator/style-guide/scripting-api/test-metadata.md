# Test Metadata Authoring Guidelines

Authoritative reference for writing `testMetadata` to validate API documentation examples. The validator executes examples via the HISE REST API and checks results.

---

## When to Mark Examples as Testable

Mark `testable: true` when the example is complete, deterministic HISEScript. Mark `testable: false` with a `skipReason` when it requires external resources that cannot be created via API (audio files, MIDI controllers, DAW interaction, hardware). Complete trivially incomplete examples before marking them non-testable.

### Module/UI References vs. External Dependencies

| Resource type | Where to create | Why |
|--------------|----------------|-----|
| **UI components** (`Content.add*()`) | Inline setup block | Environment setup |
| **Modules** (Builder API) | Inline setup block | Environment setup, see `builder_reference.md` |
| **Broadcasters** | Example code | Subject of the example, not setup |
| **Cables** | Example code | Should show full acquisition pattern |
| **External files, MIDI, hardware** | Cannot create | Mark `testable: false` |

### Callback Testability: Programmatic Triggers

The real question: **Can I trigger this callback from script?** If yes, the example is testable. Wrap all triggers in `// --- test-only ---` markers. Prefer natural API triggers over `Console.testCallback` when both options exist.

| Callback / state | Programmatic trigger | Verify with |
|-----------------|---------------------|-------------|
| Transport change | `startInternalClock(0)` / `stopInternalClock(0)` | REPL: callback log or `th.isPlaying()` |
| Tempo change | `Engine.setHostBpm(newValue)` | REPL: callback log |
| Grid configuration | `setEnableGrid(true, tempoFactor)` | REPL: `th.getGridLengthInSamples()` |
| Broadcaster listeners | `bc.sendSyncMessage([...])` | REPL: listener side-effects |
| Control callbacks | `component.setValue(x); component.changed()` | REPL: callback log or component state |
| Key press callbacks | `Console.testCallback(component, "setKeyPressCallback", eventObj)` | REPL: callback side-effects |
| Panel mouse callbacks | `Console.testCallback(panel, "setMouseCallback", eventObj)` | REPL: `panel.data` or callback side-effects |
| Cable value callbacks (sync) | `cable.setValue(x)` in test-only (fires immediately during onInit) | REPL: `reg` variable set by extra sync callback registered in test-only |
| Cable value callbacks (async) | `cable.setValue(x)` in test-only (fires on next UI tick) | REPL with `delay`: `reg` variable set by callback |
| Cable data callbacks | `secondRef.sendData(obj)` in test-only (second reference bypasses recursion guard) | log-output or REPL: callback side-effects |

`Console.testCallback` synchronously invokes a registered callback with a predetermined argument object. It supports a limited set of callback types per component - check the component's `testCallback` override in `ScriptingApiContent.h` for what's available.

> If you discover a new programmatic trigger for a callback type not listed here, add it to this table.

Mark `testable: false` only when there is truly no scriptable trigger: bypass detection (watchdog-based), DAW time signature changes, table interaction callbacks (click, selection, slider/button/combobox cell events), or hardware interaction. Audio playback tick callbacks (`setOnBeatChange`, `setOnGridChange`) may be testable with `startInternalClock` + `delay` verification but this is unproven - mark them `testable: false` for now.

---

## Markdown Source Format

Test metadata lives in phase `.md` source files. The validator reads these directly - no intermediate JSON step.

### File Locations

| `--source` | Phase | Path pattern |
|------------|-------|-------------|
| `auto` | Phase 1 | `enrichment/phase1/{Class}/methods.md` (PascalCase dir, `## method` headings) |
| `project` | Phase 2 | `enrichment/phase2/{Class}/{method}.md` (PascalCase dir, camelCase file) |
| `manual` | Phase 3 | `enrichment/phase3/{Class}/{method}.md` (PascalCase dir, lowercase file) |

Directory lookups are case-insensitive.

### Format Elements

**Slugs** - kebab-case identifier on the code fence, unique per method per phase:
````markdown
```javascript:my-example-slug
// code here
```
````

**Inline titles** - `// Title:` as first line (stripped before execution, preserved in file):
````markdown
```javascript:guard-clause-pattern
// Title: Guard clauses in a data-binding utility
Console.assertTrue(isDefined(panel));
```
````

**Inline setup scripts** - `// --- setup ---` / `// --- end setup ---` markers (extracted and run separately before the example):
````markdown
```javascript:knob-set-value
// --- setup ---
const var knob = Content.addKnob("Knob1", 0, 0);
knob.set("saveInPreset", false);
// --- end setup ---
knob.setValue(0.5);
```
````

**Inline test-only code** - `// --- test-only ---` / `// --- end test-only ---` markers (hidden from user-facing display, compiled as part of the example):
````markdown
```javascript:key-press-test
const var Viewport1 = Content.addViewport("Viewport1", 0, 0);
Viewport1.setConsumedKeyPresses("all");
Viewport1.setKeyPressCallback(onKeyPress);

// --- test-only ---
Console.testCallback(Viewport1, "setKeyPressCallback", {
    "isFocusChange": false,
    "character": "A",
    "specialKey": false,
    "keyCode": 65,
    "description": "A"
});
// --- end test-only ---
```
````

**testMetadata blocks** - JSON fence with `json:testMetadata:<slug>` matching the example's slug:
````markdown
```json:testMetadata:cable-to-gain
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "cable.getValueNormalised()", "value": 0.75}
  ]
}
```
````

**Legacy format:** Phase 3 files may use unslugged ```` ```javascript ```` fences. The reader auto-generates slugs as `{filename}-{N}` (1-based). These work for validation but cannot be targeted by `--add-metadata` or `--edit`.

---

## Test Metadata Schema

### Required Fields

- **`testable`**: boolean (required)
- **`skipReason`**: string (recommended when `testable: false`)
- **`setupScript`**: string (optional) - code to run before the example. Omit when using inline setup instead.
- **`testOnly`**: string (optional) - code compiled with the example but hidden from display. Omit when using inline test-only markers instead (preferred).
- **`verifyScript`**: object or array of objects (required if `testable: true`). Each entry supports an optional **`delay`** field (integer, milliseconds) - see Per-Step Delay below.

---

## Verification Types

### 1. Log Output (`type: "log-output"`)

```json
{"type": "log-output", "values": ["expected", "log", "entries"]}
```

**Matching rules:** Exact count match required. Type normalization (`10` matches `"10"` matches `"10.0"`). "Interface: " prefix auto-stripped. Case-sensitive. Whitespace trimmed.

### 2. REPL (`type: "REPL"`)

```json
{"type": "REPL", "expression": "myVar", "value": 42}
```

`value` field is REQUIRED. Type normalization same as log-output. `"undefined"` string handled specially. Stops at first failure.

### 3. Error (`type: "expect-error"`)

```json
{"type": "expect-error", "errorMessage": "Assertion failure: condition is false"}
```

**Matching rules:** Execution must fail. Line/column prefix auto-stripped. Case-insensitive substring match.

**Common assertion patterns:**

| Method | Error Pattern |
|--------|--------------|
| `assertTrue(false)` | `"Assertion failure: condition is false"` |
| `assertEqual(1, 2)` | `"Assertion failure: 1 != 2"` |
| `assertIsDefined(undefined)` | `"Assertion failure: variable is undefined"` |

### 4. Mixed Verification

Combine types in an array. Each object needs an explicit `"type"` field. `expect-error` MUST be last (execution stops on error).

### Per-Step Delay

Each verifyScript entry supports an optional `delay` field (integer, milliseconds) that pauses before executing that step. Default is 0 for the first step and 100ms between subsequent steps. Maximum 1000ms.

Use when async operations need time to settle: async callbacks, transport clock ticks, MidiPlayer playback position updates.

---

## HISE-Specific Setup Patterns

### Setup-Block Scope Isolation

**Problem:** The validator compiles the setup block and the example code as separate `onInit` calls (see `snippet_validator.py` lines 728-744). Variables declared in the setup block do NOT carry over to the example code or to REPL `verifyScript` expressions.

**Rule:** Setup blocks must only contain side-effect-producing code that creates persistent HISE state:
- Builder API calls (`builder.create()`, `builder.flush()`)
- Engine configuration (`Engine.setHostBpm()`)
- Transport control (`th.startInternalClock()`)

The example body must re-obtain any references it needs from the persistent state:

````markdown
```javascript:correct-scope
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.SimpleGain, "TestGain", 0, builder.ChainIndexes.FX);
builder.flush();
// --- end setup ---

// Re-obtain the reference in example scope -- do NOT reuse `builder`
const var gain = Synth.getEffect("TestGain");
gain.setAttribute(gain.Gain, -6.0);
```
````

**Common violation:** Declaring a variable in setup (e.g., `const var cable = ...`) and referencing it in the example body or in a REPL expression. This compiles fine when read as a single script but fails validation because setup and example are separate compilations.

### Creating Modules with Builder API

See `builder_reference.md` for the full Builder API reference including module types, chain indexes, and `InterfaceTypes`. Always `builder.clear()` first, always `builder.flush()` last.

### saveInPreset Pattern

**Problem:** `setValue()` during `onInit` gets overwritten by HISE's user preset model restore.

**Solution:** Set `saveInPreset: false` immediately after creating the component.

**Applies to:** All `Content.add*()` components when a test calls `setValue()` and verifies with REPL.

### Triggering Callbacks via REPL

**Problem:** `changed()` is a no-op during `onInit`, so control callbacks cannot be triggered from example code. The validator runs all example code as a single `onInit` compilation.

**Solution:** Chain `setValue()` and `changed()` with `||` in a single REPL expression, then verify the callback's side-effects in a subsequent check:

````json
"verifyScript": [
  {"type": "REPL", "expression": "Component1.setValue(1) || Component1.changed()", "value": false},
  {"type": "REPL", "expression": "callbackLog[0]", "value": 1}
]
````

The `||` operator evaluates both sides (both return undefined/falsy) and returns `false`. REPL checks run sequentially, so order is guaranteed.

### Global State: Transport and BPM

Transport state (playing/stopped) and host BPM persist across recompiles. Examples that depend on or modify these must reset them in an inline setup block:

- **BPM:** `Engine.setHostBpm(120)` explicitly resets to 120. Note: `setHostBpm(-1)` only clears the override flag but does NOT reset the BPM value - always use an explicit value.
- **Transport stopped:** `th.stopInternalClock(0)` (requires `setSyncMode(th.InternalOnly)` first)
- **Transport playing:** `th.startInternalClock(0)` (requires `setSyncMode(th.InternalOnly)` first)

---

## Verification Strategy

### Integration/Connection Examples

Integration examples MUST verify BOTH sides. Single-sided checks only prove the API call, not the integration.

| Integration Type | Verify API Call Side | Verify Effect Side |
|-----------------|---------------------|-------------------|
| Cable -> Module | `cable.getValue()` | `module.getAttribute(param)` |
| Component -> Cable | `component.getValue()` | `cable.getValue()` |
| Broadcaster -> Listener | `broadcaster.argName` | Listener state/effect |

**Recognition keywords:** "connect", "link", "drive", "sync", "route", "bind" in titles means verify both endpoints.

### Configuration Chains

Methods that configure state without producing direct output need a minimal chain to create verifiable state:
- Tables: setTableMode -> setTableColumns -> setTableRowData -> setValue -> verify getValue
- Broadcasters: createBroadcaster -> addListener -> send value -> verify listener effect

---

## Common Patterns

### Integration Test (Dual Verification)

````markdown
```javascript:cable-to-gain-integration
// Title: Cable driving a gain parameter end-to-end
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.SimpleGain, "SimpleGain1", 0, builder.ChainIndexes.FX);
builder.flush();
// --- end setup ---

const var rm = Engine.getGlobalRoutingManager();
const var cable = rm.getCable("TestCable");
cable.connectToModuleParameter("SimpleGain1", "Gain", {"MinValue": -100, "MaxValue": 0});
cable.setValueNormalised(0.75);
const var gain = Synth.getEffect("SimpleGain1");
```
```json:testMetadata:cable-to-gain-integration
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "cable.getValueNormalised()", "value": 0.75},
    {"type": "REPL", "expression": "gain.getAttribute(gain.Gain)", "value": -25.0}
  ]
}
```
````

### Table Configuration Chain

````markdown
```javascript:table-setup-and-select
// Title: Complete table setup with row selection
const var table = Content.addViewport("DataTable", 0, 0);
table.setTableMode({"RowHeight": 30, "Sortable": true});
table.setTableColumns([{"ID": "Name", "MinWidth": 150}]);
table.setTableRowData([{"Name": "A"}, {"Name": "B"}]);
table.set("saveInPreset", false);
table.setValue([0, 1]);
```
```json:testMetadata:table-setup-and-select
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "table.getValue()", "value": [0, 1]}
}
```
````

### Broadcaster Integration

````markdown
```javascript:broadcaster-chain
// Title: Broadcaster with listener and triggered message
const var bc = Engine.createBroadcaster({
    "id": "StatusBroadcaster",
    "args": ["status", "count"]
});

var log = [];

bc.addListener("logger", "Logs status changes",
function(status, count)
{
    log.push(status + ":" + count);
});

bc.sendSyncMessage(["ready", 1]);
bc.sendSyncMessage(["done", 2]);
```
```json:testMetadata:broadcaster-chain
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "log.length", "value": 2},
    {"type": "REPL", "expression": "log[0]", "value": "ready:1"},
    {"type": "REPL", "expression": "bc.status", "value": "done"}
  ]
}
```
````

### Cable Callback Testing

Cable callbacks have three distinct dispatch modes. The key challenge is the **recursion guard**: a cable reference skips its own callbacks when it sends a value/data.

**A. Sync value callback - extra registration in test-only**

When the user's callback does not produce observable output, register a second sync callback in test-only to capture the value. The extra callback must be an `inline function` - non-inline functions are silently ignored for `SyncNotification`.

````markdown
```javascript:sync-capture-pattern
const var rm = Engine.getGlobalRoutingManager();
const var cable = rm.getCable("MyCable");

inline function onCableSync(value)
{
    // Realtime-safe work only
};

cable.registerCallback(onCableSync, SyncNotification);

// --- test-only ---
reg syncResult = -1.0;
inline function testSyncCapture(v) { syncResult = v; };
cable.registerCallback(testSyncCapture, SyncNotification);
cable.setValue(0.5);
// --- end test-only ---
```
```json:testMetadata:sync-capture-pattern
{
  "testable": true,
  "verifyScript": {"expression": "syncResult", "value": 0.5}
}
```
````

**B. Async value callback - test-only setValue + delay**

Async callbacks fire on the UI thread after onInit completes. 300ms delay is sufficient for the UI timer to tick.

````markdown
```javascript:async-delay-pattern
reg currentLevel = 0.0;

inline function onLevelChanged(value)
{
    currentLevel = value;
};

cable.registerCallback(onLevelChanged, AsyncNotification);

// --- test-only ---
cable.setValue(0.75);
// --- end test-only ---
```
```json:testMetadata:async-delay-pattern
{
  "testable": true,
  "verifyScript": [{"delay": 300, "expression": "currentLevel", "value": 0.75}]
}
```
````

**C. Data callback - second cable reference bypasses recursion guard**

`sendData()` skips the sender's own data callbacks. Data callbacks dispatch synchronously during onInit (despite being documented as "asynchronous high-priority"), so `Console.print` output appears in the compilation logs.

````markdown
```javascript:data-callback-bypass
const var rm = Engine.getGlobalRoutingManager();
const var cable = rm.getCable("DataCable");

inline function onDataReceived(data)
{
    Console.print("Received: " + data.noteNumber);
};

cable.registerDataCallback(onDataReceived);

// --- test-only ---
const var triggerCable = rm.getCable("DataCable");
triggerCable.sendData({"noteNumber": 42});
// --- end test-only ---
```
```json:testMetadata:data-callback-bypass
{
  "testable": true,
  "verifyScript": {"type": "log-output", "values": ["Received: 42"]}
}
```
````

### REPL Side-Effect Assignment

When verifyScript steps need to capture a runtime value for later comparison, use `reg` variables and the `|| true` assignment pattern.

HISEScript assignments return `undefined`, which is falsy, so `|| true` always evaluates and returns `true`. The assignment still executes as a side effect.

````markdown
```javascript:lfo-value-changes
// (setup creates a GlobalModulatorContainer + LFO, connects to cable)
const var cable = rm.getCable("LfoMod");
cable.connectToGlobalModulator("TestLFO", true);

// --- test-only ---
reg v1 = 0.0;
reg v2 = 0.0;
// --- end test-only ---
```
```json:testMetadata:lfo-value-changes
{
  "testable": true,
  "verifyScript": [
    {"delay": 200, "expression": "v1 = cable.getValueNormalised() || true", "value": true},
    {"delay": 300, "expression": "v2 = cable.getValueNormalised() || true", "value": true},
    {"expression": "Math.abs(v1 - v2) > 0.01", "value": true}
  ]
}
```
````

**Rules:**
- Declare `reg` variables in the test-only block (not `var` or `const var` - `reg` survives across REPL calls)
- Use `Math.abs(a - b) > threshold` for float-safe difference checks, never `!=` on floats
- Each assignment step returns `true`, verified against `"value": true`

---

## CLI Reference

All commands require `--source` (`auto` | `project` | `manual` | `all`). Examples are addressed by `--slug` (not integer index). Working directory: `tools/api generator/`.

### Coverage

```bash
python snippet_validator.py --coverage --source all --class Console
python snippet_validator.py --coverage --source all --all-classes
```

### Extract (read-only)

```bash
python snippet_validator.py --extract --source project --class Console --method assertTrue
python snippet_validator.py --extract --source project --class Console --method assertTrue --slug guard-clause-pattern
python snippet_validator.py --extract --source project --class Console --method assertTrue --show-metadata
```

### Edit

```bash
python snippet_validator.py --edit --source project --class Console --method assertLegalNumber \
  --slug detecting-division-by-zero \
  --code $'var inputLevel = 0.0;\nvar gain1 = 1.0 / inputLevel;\n\nConsole.assertLegalNumber(gain1);'
```

### Add Metadata

```bash
# Non-testable (always include --skip-reason)
python snippet_validator.py --add-metadata --source project --class Array --method reserve \
  --slug pre-allocating-capacity --testable false \
  --skip-reason "Pseudo-code showing allocation pattern, not a complete runnable script"

# log-output
python snippet_validator.py --add-metadata --source project --class Array --method concat \
  --slug merging-arrays --testable true --verify-type log-output \
  --verify-values '[3, "[0, 1, 2, 3, 4, 5]"]'

# REPL
python snippet_validator.py --add-metadata --source project --class Array --method clone \
  --slug shallow-copy --testable true --verify-type REPL \
  --verify-checks '[{"expression": "arr2[0]", "value": 22}]'

# expect-error
python snippet_validator.py --add-metadata --source project --class Console --method assertTrue \
  --slug unreachable-code --testable true --verify-type expect-error \
  --verify-error-message "Assertion failure: condition is false"
```

### Validate

```bash
# Full class validation (auto-launches HISE)
python snippet_validator.py --validate --source all --class Console --launch

# Keep HISE running between iterative runs
python snippet_validator.py --validate --source all --class Console --launch --keep-alive

# Single example by slug
python snippet_validator.py --validate --source project --class Console --method assertEqual --slug verifying-data-array-dimensions

# Use release build on custom port
python snippet_validator.py --validate --source all --class Console --launch --no-debug --port 1901
```

**Launch flags** (only with `--validate`):

| Flag | Description |
|------|-------------|
| `--launch` | Auto-launch HISE before validation, shut down after |
| `--keep-alive` | Keep HISE running after validation (requires `--launch`) |
| `--no-debug` | Use release build (`HISE.exe`) instead of debug (`HISE Debug.exe`) |
| `--port PORT` | REST API port (default: 1900) |

### Shutdown

```bash
python snippet_validator.py --shutdown
```

**Exit codes:** 0 = all passed, 1 = failures, 2 = connection lost mid-run (partial results saved), 3 = launch failed.

---

## Pipeline Integration

Test metadata lives in phase `.md` source files and follows examples through the merge into `api_reference.json`.

**Merge behavior:** `api_enrich.py merge` reads the sidecar (`enrichment/output/test_results.json`) during example selection:
- Failed examples (`tested: true, passed: false`) are discarded
- If all winning-phase examples fail, merge falls back to a lower phase
- Untested examples are kept (absence from sidecar = "unknown, might work")

**Sidecar keying:** `Class.method.source.slug` (e.g., `Console.assertEqual.project.verifying-data-array-dimensions`). Results accumulate across runs.

---

## Related Guidelines

- `builder_reference.md` - Builder API reference (module types, chain indexes, InterfaceTypes)
- `hisescript_example_rules.md` - HISEScript syntax rules (inline function, const var, etc.)
- `code_example_quality.md` - Editorial quality (lead with non-obvious behavior, show edge cases, obtain your objects)
