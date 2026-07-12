Attempts to cast this ChildSynth to a Sampler reference. If the wrapped module is a ModulatorSampler, returns a Sampler handle that gives access to sample map loading, round-robin configuration, and other sampler-specific methods. If the module is not a sampler type, returns undefined silently.

> [!Warning:Check return with isDefined first] Always check the return value with `isDefined()` before calling Sampler methods. The cast returns undefined without error when the child is not a sampler, which will cause a runtime error if you chain method calls directly.
