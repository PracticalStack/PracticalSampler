Loads a sample map from an SFZ file. Accepts a `File` object or an absolute path string. Returns `undefined` on success or an error string on failure - check with `isDefined(result)` to detect errors.

The SFZ importer handles mapping information (key ranges, velocity layers, round-robin groups) but does not parse synthesis parameters like envelope times. SFZ compliance is partial; report parsing issues on the HISE forum.

> [!Warning:SFZ maps not auto-persisted in presets] The sampler does not store SFZ-loaded maps automatically. Add a UI component (e.g. a ScriptPanel with `ScriptPanel.setFileDropCallback()`) that persists the file reference and reloads in its control callback.
