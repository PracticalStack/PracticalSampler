# ExpansionHandler -- Class Analysis

## Brief
Factory and manager for loading, installing, encrypting, and switching expansion packs at runtime.

## Purpose
ExpansionHandler is the scripting API wrapper for HISE's expansion pack system. It manages the lifecycle of expansion packs -- discovering them in the Expansions folder, installing new ones from `.hr` archive packages, setting credentials for encrypted expansions, and switching the active expansion at runtime. It provides callbacks for expansion load events, installation progress, and error handling. Created via `Engine.createExpansionHandler()`, it delegates all operations to the core `ExpansionHandler` singleton on the MainController.

## Details

### Expansion Type System

HISE supports three expansion types, exposed as integer constants on the ExpansionHandler object:

| Type | File Format | Description |
|------|-------------|-------------|
| FileBased (0) | `expansion_info.xml` | Plain folder structure with unencrypted resources. Development format. |
| Intermediate (1) | `.hxi` | Encoded ValueTree containing pool data. Standard distribution format. |
| Encrypted (2) | `.hxp` | Credential-encrypted `.hxi`. Requires `setCredentials()` to decode. |

Use `setAllowedExpansionTypes()` to restrict which types can load. Disallowed types move to the uninitialised list and become invisible to `getExpansionList()`.

### Installation Pipeline

The `installExpansionFromPackage()` method runs on the SampleLoadingThread (voices are killed first). The pipeline:

1. Reads package metadata to determine the target expansion folder name
2. Creates the expansion directory structure under `{ProjectRoot}/Expansions/`
3. Optionally creates a link file if the sample directory differs from the default
4. Decompresses the `.hr` archive (HLAC format) into the sample directory
5. If credentials are set, encrypts `header.dat` into `.hxp`; otherwise renames to `.hxi`
6. Reinitialises all expansions and notifies listeners

### Install Callback Object

See `setInstallCallback()` for the full callback properties table and firing behavior.

### Credential vs Encryption Key

Two distinct protection mechanisms exist. See `setCredentials()` for the current recommended approach (JSON credentials for `.hxp` expansions) and `setEncryptionKey()` for the deprecated BlowFish key method (always throws an error; configure in Project Settings instead).

### Error Function Signature

See `setErrorFunction()` for the callback signature and error routing details.

### Expansion Callback Behavior

See `setExpansionCallback()` for the full list of events that trigger the callback and its argument format.

### setCurrentExpansion Polymorphism

See `setCurrentExpansion()` for the polymorphic argument handling (String name, Expansion reference, or empty string to clear).

## obtainedVia
`Engine.createExpansionHandler()`

## minimalObjectToken
eh

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| FileBased | 0 | int | Plain folder expansion with expansion_info.xml | ExpansionType |
| Intermediate | 1 | int | Encoded .hxi expansion format | ExpansionType |
| Encrypted | 2 | int | Credential-encrypted .hxp expansion format | ExpansionType |

## Dynamic Constants
None.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `eh.setEncryptionKey("myKey")` | Configure encryption key in Project Settings | `setEncryptionKey` is hard-deprecated and always throws an error. |
| `eh.setCredentials("keyString")` | `eh.setCredentials({"key": "value"})` | `setCredentials` requires a JSON object, not a string. Passing a non-object triggers an error via the error function. |
| `eh.setAllowedExpansionTypes(eh.Encrypted)` | `eh.setAllowedExpansionTypes([eh.Encrypted])` | `setAllowedExpansionTypes` requires an array of type constants, not a single value. |

## codeExample
```javascript
// Create the expansion handler
const var eh = Engine.createExpansionHandler();

// Set up callbacks
eh.setExpansionCallback(function(e)
{
    if (isDefined(e))
        Console.print("Loaded expansion: " + e.getProperties().Name);
    else
        Console.print("No expansion active");
});

eh.setErrorFunction(function(message, isCritical)
{
    Console.print((isCritical ? "ERROR: " : "Warning: ") + message);
});

// Get all available expansions
const var list = eh.getExpansionList();
```

## Alternatives
- `Expansion` -- Handle to a single installed expansion pack providing access to its sample maps, audio files, images, and data files. ExpansionHandler is the factory that returns these references.
- `Unlocker` -- RSA key-based product licensing. Unlocker manages product-level licensing; ExpansionHandler manages expansion pack installation and encryption which may use Unlocker credentials.

## Related Preprocessors
`USE_BACKEND`, `HISE_USE_CUSTOM_EXPANSION_TYPE`, `HISE_USE_UNLOCKER_FOR_EXPANSIONS`, `HISE_USE_XML_FOR_HXI`

## Diagrams
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: ExpansionHandler methods have clear error reporting through the error function callback and script errors. No silent-failure preconditions or timeline dependencies that would benefit from parse-time diagnostics.
