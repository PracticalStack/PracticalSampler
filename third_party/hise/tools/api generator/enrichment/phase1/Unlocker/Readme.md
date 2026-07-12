# Unlocker -- Class Analysis

## Brief
RSA key-based product license manager for registration, key file handling, and expansion unlocking.

## Purpose
The Unlocker provides scripted access to HISE's RSA-based copy protection system, built on top of JUCE's `OnlineUnlockStatus`. It handles license key file validation, storage, and retrieval, as well as user identity queries (email, machine ID). When configured with `HISE_USE_UNLOCKER_FOR_EXPANSIONS`, it also manages per-expansion licensing through encrypted expansion list files. The class supports time-limited licenses via RSA-encrypted expiration data and optional MuseHub SDK integration.

## Details

### Two-Layer Architecture

The scripting API object (`Unlocker`) is actually a lightweight `RefObject` wrapper that holds a weak reference to the real `ScriptUnlocker` singleton. The `ScriptUnlocker` is owned by the processor (`BackendProcessor` or `FrontendProcessor`) and persists across script recompilations. The `RefObject` registers itself as `currentObject` on the `ScriptUnlocker` to receive callbacks (product check, MuseHub result).

### Preprocessor Configuration

| Preprocessor | Default | Effect |
|---|---|---|
| `USE_COPY_PROTECTION` | 0 | Enables the copy protection system in frontend builds |
| `USE_SCRIPT_COPY_PROTECTION` | 0 | Uses ScriptUnlocker (not built-in UI) for frontend copy protection. Implies `USE_COPY_PROTECTION = 1` |
| `HISE_USE_UNLOCKER_FOR_EXPANSIONS` | 0 | Enables expansion list management through the Unlocker |
| `HISE_INCLUDE_MUSEHUB` | undefined | Enables MuseHub SDK integration |

When `USE_SCRIPT_COPY_PROTECTION` is enabled, the built-in unlocker overlay (`HISE_INCLUDE_UNLOCKER_OVERLAY`) is disabled, giving the script full control over the registration UI.

### Key File Format

License key files follow the JUCE RSA key file format:
- Must start with `"Keyfile for "`
- Contains a `"Machine numbers: "` line with the registered machine ID
- Contains an RSA-encrypted blob validated against the project's public key

### Product ID Matching

By default, version numbers are stripped from product ID comparisons. A key file generated for "MyPlugin 1.0" will still validate for "MyPlugin 2.0". Use `setProductCheckFunction()` to override this with custom version-aware matching.

### Expansion List System

When `HISE_USE_UNLOCKER_FOR_EXPANSIONS` is enabled:
- The expansion list file is stored as a sibling to the main license key file (named "expansions" with the same extension)
- Expansion data uses double encryption: RSA public key + BlowFish (keyed with machine ID)
- The decrypted payload is XML containing email, machine_id, product, and per-expansion slug/key pairs
- Credentials are passed to `ExpansionHandler.setCredentials()` to unlock expansions

### Auto-Loading on Construction

The `RefObject` constructor automatically:
1. Loads the license key file if it exists on disk
2. Loads the expansion list if `HISE_USE_UNLOCKER_FOR_EXPANSIONS` is enabled and the plugin is unlocked

### Backend vs Frontend Behavior

- `unlockExpansionList()` is backend-only -- it scans expansion folders for metadata and collects encryption keys
- `checkMuseHub()` is simulated in backend (random result after 2s delay) and uses the real SDK in frontend
- `getPublicKey()` reads from the project's RSA key file in backend; in frontend with `USE_SCRIPT_COPY_PROTECTION`, the user must provide the implementation

## obtainedVia
`Engine.createLicenseUnlocker()`

## minimalObjectToken
ul

## Constants

No constants. The constructor passes 0 to the base class.

## Dynamic Constants

No dynamic constants.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `ul.loadExpansionList()` without `HISE_USE_UNLOCKER_FOR_EXPANSIONS` | Enable `HISE_USE_UNLOCKER_FOR_EXPANSIONS` in projucer settings | Calling expansion methods without the preprocessor flag causes a script error |
| `ul.writeExpansionKeyFile(data)` where data doesn't start with `"Expansion List"` | Ensure keyData starts with `"Expansion List"` | The method silently returns false if the header prefix is missing |
| Using `Unlocker` in frontend without `USE_COPY_PROTECTION` | Enable `USE_COPY_PROTECTION` (or `USE_SCRIPT_COPY_PROTECTION`) | The license unlocker singleton does not exist in frontend builds without copy protection enabled |

## codeExample
```javascript
// Create a license unlocker reference
const ul = Engine.createLicenseUnlocker();

// Check registration status
if (ul.isUnlocked())
    Console.print("Licensed to: " + ul.getUserEmail());
```

## Alternatives
BeatportManager -- uses Beatport SDK instead of RSA key files for DRM. ExpansionHandler -- manages expansion packs but delegates licensing to Unlocker when `HISE_USE_UNLOCKER_FOR_EXPANSIONS` is enabled.

## Related Preprocessors
`USE_COPY_PROTECTION`, `USE_SCRIPT_COPY_PROTECTION`, `HISE_USE_UNLOCKER_FOR_EXPANSIONS`, `HISE_INCLUDE_MUSEHUB`

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Unlocker methods either return status values or throw script errors for misconfiguration -- no silent-failure preconditions that would benefit from parse-time diagnostics.
