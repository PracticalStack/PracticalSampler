Enables caching of resynthesis results in the specified directory. After enabling the cache, any resynthesis result is stored as a `.tmp` file keyed on the source filename and the current resynthesis options. Subsequent loads with the same file and options skip the resynthesis step entirely.

The `clearCache` parameter deletes all existing `.tmp` files in the cache directory before setting the new path, which is useful during development when options change frequently.

> [!Warning:Post-FX not included in cache] The cache stores only the resynthesised wavetable data before post-processing. Post-FX processors set via `setPostFXProcessors()` are re-applied every time the wavetable is loaded from cache.
