<!-- Diagram triage:
  - No diagrams specified in Phase 1 data
-->

# Unlocker

Unlocker manages RSA key-based copy protection for your plugin. It handles the full licence lifecycle: writing key files received from your server, loading and validating them against the project's public RSA key, and querying registration status (email, machine ID, unlock state).

The class supports three licensing models:

1. **Standard RSA key file** - a signed key file validated against the project's public key.
2. **Time-limited licence** - an RSA-encrypted expiration timestamp that grants access for a set number of days.
3. **MuseHub SDK** - an asynchronous licence check through the MuseHub platform.

When expansion licensing is enabled (`HISE_USE_UNLOCKER_FOR_EXPANSIONS`), the Unlocker also manages per-expansion credentials through a separate encrypted expansion list file that sits alongside the main licence key file.

```js
const ul = Engine.createLicenseUnlocker();
```

The typical registration workflow is:

1. Check `keyFileExists()` to decide whether to show a registration screen.
2. Receive key data from your server and validate its format with `isValidKeyFile()`.
3. Write it to disk with `writeKeyFile()`, then call `loadKeyFile()` to validate and unlock.
4. Query `isUnlocked()` to gate plugin functionality.

The following preprocessor flags control which features are available:

| Preprocessor | Effect |
|---|---|
| `USE_COPY_PROTECTION` | Enables the copy protection system in frontend builds |
| `USE_SCRIPT_COPY_PROTECTION` | Uses the scripted Unlocker (not the built-in overlay) for frontend copy protection |
| `HISE_USE_UNLOCKER_FOR_EXPANSIONS` | Enables expansion list management through the Unlocker |
| `HISE_INCLUDE_MUSEHUB` | Enables MuseHub SDK integration |

> The Unlocker singleton persists across script recompilations. A key file present on disk is loaded automatically when `Engine.createLicenseUnlocker()` is called, so the plugin can be unlocked before any manual `loadKeyFile()` call.

> By default, version numbers are stripped from product ID matching. A key file generated for "MyPlugin 1.0" will still validate "MyPlugin 2.0". Use `setProductCheckFunction()` to override this with version-aware matching.

## Common Mistakes

- **Enable copy protection before using the Unlocker in exports**
  **Wrong:** Using `Unlocker` in a frontend build without `USE_COPY_PROTECTION`
  **Right:** Enable `USE_COPY_PROTECTION` (or `USE_SCRIPT_COPY_PROTECTION`) in the Projucer settings
  *The licence unlocker singleton does not exist in frontend builds unless copy protection is enabled. Calls will fail silently or throw errors.*

- **Enable the expansion preprocessor before calling expansion methods**
  **Wrong:** `ul.loadExpansionList()` without `HISE_USE_UNLOCKER_FOR_EXPANSIONS`
  **Right:** Enable `HISE_USE_UNLOCKER_FOR_EXPANSIONS` in the Projucer settings before calling expansion methods
  *Expansion methods throw a script error if the preprocessor flag is not set. This is a build configuration issue, not a runtime error.*

- **Validate the expansion key data header**
  **Wrong:** `ul.writeExpansionKeyFile(data)` where `data` does not start with `"Expansion List"`
  **Right:** Ensure the key data string starts with `"Expansion List"` before writing
  *The method silently returns false if the header prefix is missing, with no error message.*
