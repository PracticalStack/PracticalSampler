# FixObjectArray -- Project Context

## Project Context

### Real-World Use Cases
- **Fixed-size object pool for real-time visualization**: A plugin that needs to track a fixed number of visual elements (particles, note trails, grain indicators) uses FixObjectArray as a pre-allocated pool. Each element holds typed properties (position, gain, state flags) that are updated every timer tick via for-in iteration, then bulk-extracted to Buffers with `copy()` for GPU shader upload or DSP processing. The fixed size guarantees no allocation during the animation loop.

### Complexity Tiers
1. **Object pool with iteration** (most common): `createArray` + bracket indexing + for-in loop to read/modify elements. Covers any use case where a fixed number of typed records need per-frame updates.
2. **Column extraction pipeline**: Adds `copy()` to extract individual properties into Buffers. Used when downstream systems (shaders, DSP networks) consume flat float arrays rather than structured objects.
3. **Persistent state**: Adds `toBase64`/`fromBase64` for saving and restoring array state across sessions. Requires careful layout versioning since the serialization is binary.

### Practical Defaults
- Use FixObjectArray when all slots are always meaningful (e.g., a pool of particles where inactive ones just have `gain = 0`). Use FixObjectStack when elements are dynamically inserted and removed.
- Set the compare function on the factory before creating the array if you plan to use `indexOf`, `contains`, or `sort`. The default byte-level comparator is rarely what you want.
- Match Buffer sizes to the array's `length` constant when using `copy()`. A size mismatch produces a script error.

### Integration Patterns
- `FixObjectArray.copy()` -> `Buffer` -> `ScriptShader.setUniformData()` -- Extract property columns into Buffers, then pass them as shader uniform data for GPU-driven visualization. Multiple `copy()` calls extract different properties into separate Buffers in the same timer tick.
- `for (obj in array)` + property modification -> `FixObjectArray.copy()` -- Update element properties in-place via for-in (live references), then bulk-extract the updated values. This two-phase pattern (mutate, then extract) avoids per-element Buffer writes.
- `FixObjectFactory.setCompareFunction()` -> `FixObjectArray.indexOf()` / `sort()` -- The compare function must be set on the factory, which propagates it to all arrays. Set it once before creating arrays or searching/sorting.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Creating separate Buffers of different sizes than the array | `const var buf = Buffer.create(array.length);` matching array size | `copy()` requires the Buffer size to exactly match the array's `length` constant. Use the same size constant for both. |
| Using a regular Array to track real-time visual state | FixObjectArray with typed properties | Regular Arrays allocate on modification. FixObjectArray's contiguous pre-allocated memory is safe for timer callbacks that run every 30ms. |
