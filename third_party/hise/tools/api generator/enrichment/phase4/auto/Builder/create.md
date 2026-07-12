Creates a new module and adds it to a chain of the parent module. Returns the new module's build index for use with other Builder methods.

The four parameters are:
- `type` - a type string from one of the dynamic constants (`b.SoundGenerators.StreamingSampler`, `b.Effects.SimpleGain`, etc.)
- `id` - a unique string identifier for the module
- `rootBuildIndex` - the build index of the parent module (0 for MainSynthChain)
- `chainIndex` - which chain to add to, using `b.ChainIndexes` constants (`Direct`, `Midi`, `Gain`, `Pitch`, `FX`)

Creation is idempotent: if a processor with the same ID already exists under the parent, its existing build index is returned without creating a duplicate. This makes Builder scripts safe to re-run.

> [!Warning:Silent reuse on ID collision] If a module with the given ID already exists, `create()` silently returns the existing module's index with no warning. This can mask accidental ID collisions - if two modules unintentionally share an ID, only the first is created and the second `create()` call quietly returns a reference to it.
