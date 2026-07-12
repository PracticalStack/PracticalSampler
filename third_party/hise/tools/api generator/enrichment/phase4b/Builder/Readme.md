Builder (object)
Obtain via: Synth.createBuilder()

Programmatic module tree construction tool. Creates, configures, and destroys
synths, effects, modulators, and MIDI processors using a build-index addressing
system. All module creation is restricted to onInit. The write-side counterpart
to Synth.get*() -- it constructs what those methods later retrieve.

Dynamic constants:
  SoundGenerators    JSON of all registered synth type names
  Modulators         JSON of all registered modulator type names
  Effects            JSON of all registered effect type names
  MidiProcessors     JSON of all registered MIDI processor type names
  InterfaceTypes     JSON mapping wrapper class names for use with get():
                       MidiProcessor, Modulator, Synth, Effect,
                       AudioSampleProcessor, SliderPackProcessor,
                       TableProcessor, Sampler, MidiPlayer, RoutingMatrix, SlotFX
  ChainIndexes       Direct=-1, Midi=0, Gain=1, Pitch=2, FX=3, GlobalMod=1

Complexity tiers:
  1. Basic: create, setAttributes, flush. Create a handful of modules with
     configured parameters.
  2. Intermediate: + get (typed references for routing/SlotFX/modulator config),
     + clearChildren (targeted chain cleanup), + connectToScript (external MIDI
     script linking).
  3. Advanced: + getExisting (reference pre-existing modules in multi-pass
     builds), loop-based construction of N identical channels, RoutingMatrix
     manipulation, conditional build flags.

Practical defaults:
  - Use ChainIndexes.Direct (-1) for adding sound generators to containers,
    ChainIndexes.FX (3) for effects. These are the two most common targets.
  - Always bracket builds with clear() at start and flush() at end. The
    clear-build-flush pattern ensures a clean slate and proper UI updates.
  - Comment out Builder code by default. Uncomment only when the module tree
    needs modification, run once, then re-comment. The tree persists in XML.
  - For large builds with distinct subsections, use boolean flags to
    enable/disable each section independently during development.

Common mistakes:
  - Forgetting flush() after create/clear -- leaves patch browser and UI out
    of sync. The destructor will warn but the damage is done.
  - Calling create() outside onInit -- throws a script error. Module creation
    is restricted to onInit via interfaceCreationAllowed().
  - Running Builder code on every compile -- unnecessary and slow. The module
    tree persists in the XML preset; rebuild only when needed.
  - Using clearChildren() on a chain containing the calling script processor
    -- no self-preservation check (unlike clear()), may cause undefined
    behavior.

Example:
  // Builder code -- uncomment to rebuild, then re-comment
  const var b = Synth.createBuilder();
  b.clear();
  var synthIdx = b.create(b.SoundGenerators.SineSynth, "MySine", 0, b.ChainIndexes.Direct);
  b.setAttributes(synthIdx, {"OctaveTranspose": 5});
  var fxIdx = b.create(b.Effects.SimpleReverb, "MyReverb", synthIdx, b.ChainIndexes.FX);
  b.flush();

Methods (8):
  clear              clearChildren      connectToScript
  create             flush              get
  getExisting        setAttributes
