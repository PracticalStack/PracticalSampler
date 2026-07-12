<!-- Diagram triage:
  - No class-level or method-level diagrams in the JSON. Nothing to render.
-->

# ExpansionHandler

ExpansionHandler manages expansion packs at runtime - discovering them in the Expansions folder, installing new ones from `.hr` archive packages, handling credentials for encrypted content, and switching the active expansion. Create one with `Engine.createExpansionHandler()`:

```javascript
const var eh = Engine.createExpansionHandler();
```

HISE supports three expansion types, controlled by `setAllowedExpansionTypes()`:

| Constant | Value | Description |
|---|---|---|
| `ExpansionHandler.FileBased` | 0 | Plain folder with `expansion_info.xml`. Development format with unencrypted resources. |
| `ExpansionHandler.Intermediate` | 1 | Encoded `.hxi` file containing pooled data. Standard distribution format. |
| `ExpansionHandler.Encrypted` | 2 | Credential-encrypted `.hxp` file. Requires `setCredentials()` to decode. |

Typical usage follows one of four complexity tiers:

1. **Browsing and switching** - build a selector from `getExpansionList()`, react to changes via `setExpansionCallback()`, switch with `setCurrentExpansion()`
2. **Resource enumeration** - use the `Expansion` object to browse sample maps, audio files, images, and MIDI files within an expansion
3. **Installation pipeline** - install `.hr` packages with `installExpansionFromPackage()`, track progress with `setInstallCallback()`
4. **Encrypted distribution** - set credentials with `setCredentials()`, encode with `encodeWithCredentials()`, restrict types with `setAllowedExpansionTypes()`

> The ExpansionHandler wrapper must be created in `onInit` and stored in a persistent variable. In earlier versions of HISE, expansion functionality was globally available; it now requires an explicit wrapper object whose callbacks (error function, expansion callback) are owned by that wrapper and go out of scope when it is destructed.

## Common Mistakes

- **Wrong:** Setting up the expansion callback after `setCurrentExpansion()`
  **Right:** Register `setExpansionCallback()` first, then call `setCurrentExpansion()`
  *The callback will not fire for the initial switch if it has not been registered yet.*

- **Wrong:** `eh.setCredentials("keyString")`
  **Right:** `eh.setCredentials({"key": "value"})`
  *`setCredentials` requires a JSON object. Passing a non-object triggers the error function rather than a script error, so the failure is silent if no error function is set.*

- **Wrong:** `eh.setAllowedExpansionTypes(eh.Encrypted)`
  **Right:** `eh.setAllowedExpansionTypes([eh.Encrypted])`
  *`setAllowedExpansionTypes` requires an array of type constants, not a single value.*

- **Wrong:** Assuming the expansion callback argument is always valid
  **Right:** Always check `isDefined(e)` in the expansion callback
  *The callback receives `undefined` when the expansion is cleared via `setCurrentExpansion("")`. Call your callback manually with `undefined` at init time to set the default UI state.*
