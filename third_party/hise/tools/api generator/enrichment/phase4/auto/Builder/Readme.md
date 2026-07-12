<!-- Diagram triage:
  - builder-create-flow: CUT (linear create-resolve-append flow is adequately described in prose; no branching or fan-out that benefits from a visual)
-->

# Builder

The Builder constructs and configures the HISE module tree from script. It creates sound generators, effects, modulators, and MIDI processors programmatically, replacing manual point-and-click module setup in the IDE.

The Builder is a development-time tool for assembling repetitive or complex module trees. Typical use cases include:

1. Building N identical channel strips in a loop (container, synth, MIDI processors, effects)
2. Scaffolding a modulation matrix with per-oscillator modulator targets
3. Constructing master and send effect chains with per-channel send routing

Obtain a Builder instance with:

```javascript
const var b = Synth.createBuilder();
```

Every call to `create()` or `getExisting()` returns a **build index** - an integer that identifies the module within this Builder session. Index 0 is always the MainSynthChain. Pass build indexes to `get()`, `setAttributes()`, `clearChildren()`, and `connectToScript()` to operate on specific modules.

Module types are selected from dynamic constant objects on the Builder instance:

| Constant | Contents |
|----------|----------|
| `b.SoundGenerators` | All registered synth types |
| `b.Modulators` | All registered modulator types |
| `b.Effects` | All registered effect types |
| `b.MidiProcessors` | All registered MIDI processor types |
| `b.ChainIndexes` | Named chain slots: `Direct` (-1), `Midi` (0), `Gain` (1), `Pitch` (2), `FX` (3) |
| `b.InterfaceTypes` | Typed wrapper names for use with `get()` |

> Builder code is a development tool, not a runtime feature. The recommended workflow is: uncomment the build code, compile once to rebuild the module tree, then re-comment it. The module tree persists in the XML preset - there is no need to rebuild on every compile. Never ship Builder calls in a compiled plugin; they are deactivated in exported builds.

> All module creation is restricted to the `onInit` callback. Calling `create()` from any other callback will throw a script error.

## Conditional Compilation

Put all Builder code in a separate file and control execution by commenting or uncommenting the `include()` statement in your main script. This keeps the build logic isolated and avoids accidentally rebuilding the module tree on every compile:

```javascript
// Interface.js

// Uncomment to rebuild the module tree, then re-comment:
//include("Builder.js");

// ... rest of your interface code
```

Inside the Builder file, use boolean flags to control which sections get rebuilt. This lets you iterate on one part of the tree without tearing down everything:

```javascript
// Builder.js
const var b = Synth.createBuilder();

const var BUILD_CHANNELS = 1;
const var BUILD_MASTER_FX = 0;
const var BUILD_SENDS = 0;

b.clear();

if(BUILD_CHANNELS)
    buildChannels(b);

if(BUILD_MASTER_FX)
    buildMasterFX(b);

if(BUILD_SENDS)
    buildSendEffects(b);

b.flush();
```

The module tree persists in the XML preset, so the Builder only needs to run when you change the structure. Keeping it commented out by default also avoids the stability risks of clearing and recreating modules on every compile.

## Common Mistakes

- **Wrong:** `b.create(...); // no flush at the end`
  **Right:** `b.create(...); b.flush();`
  *Forgetting `flush()` leaves the patch browser and UI out of sync with the actual module tree. The Builder's destructor will log a warning if you forget.*

- **Wrong:** Running Builder code on every compile
  **Right:** Commenting out the build call (or its `include` statement) after use
  *Rebuilding the entire module tree on every compile is slow and unnecessary. The tree persists in the preset XML. Uncomment only when you need to modify the structure.*

- **Wrong:** Building N identical channels by hand in the IDE
  **Right:** Using a loop with indexed IDs (e.g. `"Sampler " + (i + 1)`)
  *Manual construction is error-prone for repetitive structures. A loop trivially creates N identical processor chains, and adding a new module to every channel becomes a one-line change.*

- **Wrong:** Creating all modules in one monolithic block
  **Right:** Splitting into per-section build functions with conditional flags
  *Boolean flags (`BUILD_CHANNELS`, `BUILD_MASTER_FX`, `BUILD_SENDS`) let you iterate on one section without rebuilding the entire tree.*
