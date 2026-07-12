Analyses an audio file using the Loris partial-tracking algorithm. The root frequency is the estimated fundamental of the sound in Hz and guides harmonic partial tracking. Returns true on success.

This must be the first operation in any Loris workflow - all extraction and processing methods require a prior analysis of the same file. When caching is enabled (the default), re-analysing the same file reuses the cached partial list. Disable caching with `set("enablecache", false)` to force a fresh analysis.
