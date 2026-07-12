# Unlocker -- Method Documentation

## canExpire

**Signature:** `bool canExpire()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var expires = {obj}.canExpire();`

**Description:**
Returns whether the current license has an expiration time set. This checks the JUCE `OnlineUnlockStatus::getExpiryTime()` value -- if it is non-zero, the license can expire. Use `checkExpirationData()` to actually validate and extend an expiration-based license.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Returns false when the unlocker reference is null (e.g., if the `ScriptUnlocker` singleton was not created), rather than throwing an error. This makes it safe to call defensively but means a false return does not distinguish between "license has no expiration" and "unlocker unavailable".

**Cross References:**
- `$API.Unlocker.checkExpirationData$`

## checkExpirationData

**Signature:** `var checkExpirationData(String encodedTimeString)`
**Return Type:** `NotUndefined`
**Call Scope:** unsafe
**Call Scope Note:** RSA decryption, ISO8601 parsing, and string operations allocate.
**Minimal Example:** `var daysLeft = {obj}.checkExpirationData(encodedHexString);`

**Description:**
Validates an RSA-encrypted expiration timestamp and, on success, unlocks the plugin for the encoded duration. The `encodedTimeString` must be a hex string starting with `"0x"` that, when RSA-decrypted with the project's public key, yields an ISO 8601 date string. On success, returns the number of days remaining (as an integer). On failure, returns `false`. If the input does not start with `"0x"`, returns the string `"encodedTimeString data is corrupt"`. If the unlocker reference is null, returns the string `"No unlocker"`.

In frontend builds, a successful call also triggers sample reloading.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| encodedTimeString | String | no | Hex-encoded RSA-encrypted ISO 8601 expiration timestamp | Must start with `"0x"` |

**Pitfalls:**
- The return type is polymorphic: Integer (days remaining) on success, `false` on RSA failure, or a String error message on format/null errors. Callers must check the type of the return value, not just its truthiness -- a string error message is truthy.

**Cross References:**
- `$API.Unlocker.canExpire$`
- `$API.Unlocker.checkMuseHub$`

**Example:**
```javascript:check-expiration
// Title: Validate an expiration-based license
const var ul = Engine.createLicenseUnlocker();

// encodedTimeString would come from your server
var result = ul.checkExpirationData(encodedHexFromServer);

if (typeof result == "number")
    Console.print("License valid for " + result + " days");
else if (result == false)
    Console.print("Expiration check failed");
else
    Console.print("Error: " + result);
```
```json:testMetadata:check-expiration
{
  "testable": false,
  "skipReason": "Requires a valid RSA-encrypted expiration hex string from a configured key pair"
}
```

## checkMuseHub

**Signature:** `void checkMuseHub(Function resultCallback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** WeakCallbackHolder construction and timer/SDK setup allocate.
**Minimal Example:** `{obj}.checkMuseHub(onMuseHubResult);`

**Description:**
Initiates an asynchronous MuseHub license check. The `resultCallback` is called with a single boolean argument indicating whether the MuseHub check succeeded. In backend builds, this is simulated with a random result after a 2-second delay. In frontend builds with `HISE_INCLUDE_MUSEHUB` enabled, this calls the real MuseHub SDK. If the check succeeds in a backend build with `JUCE_ALLOW_EXTERNAL_UNLOCK`, the plugin is also unlocked via `unlockExternal()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| resultCallback | Function | no | Callback invoked with the check result | Must accept 1 argument |

**Callback Signature:** resultCallback(ok: bool)

**Pitfalls:**
- In backend builds, the result is random (50% chance of success) with a fixed 2-second delay. Do not use backend results for testing real MuseHub integration logic.
- The callback is stored as a `WeakCallbackHolder`. If the scripting object is destroyed before the async result arrives, the callback is silently dropped.

**Cross References:**
- `$API.Unlocker.isUnlocked$`
- `$API.Unlocker.checkExpirationData$`

**Example:**
```javascript:musehub-check
// Title: Check MuseHub license asynchronously
const var ul = Engine.createLicenseUnlocker();

inline function onMuseHubResult(ok)
{
    if (ok)
        Console.print("MuseHub license verified");
    else
        Console.print("MuseHub check failed");
}

ul.checkMuseHub(onMuseHubResult);
```
```json:testMetadata:musehub-check
{
  "testable": false,
  "skipReason": "Requires MuseHub SDK or produces random backend simulation results"
}
```

## contains

**Signature:** `bool contains(String otherString)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations.
**Minimal Example:** `var found = {obj}.contains("FeatureX");`

**Description:**
Checks whether the loaded key file data contains the given substring. This delegates to the JUCE `OnlineUnlockStatus` key file content search. Can be used to check for feature flags or product tiers embedded in the key file data.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| otherString | String | no | Substring to search for in the key file data | -- |

**Pitfalls:**
- [BUG] Returns true when the unlocker reference is null, rather than false. This permissive fallback means an uninitialized or unavailable unlocker appears to contain everything, which could bypass feature checks if not guarded by an `isUnlocked()` call first.

**Cross References:**
- `$API.Unlocker.isUnlocked$`
- `$API.Unlocker.loadKeyFile$`

## getLicenseKeyFile

**Signature:** `String getLicenseKeyFile()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations on the returned path string.
**Minimal Example:** `var path = {obj}.getLicenseKeyFile();`

**Description:**
Returns the full file path of the license key file. In backend builds, this is `{AppDataRoot}/{Company}/{Project}/{Project}.{extension}`. In frontend builds, it delegates to `FrontendHandler::getLicenseKey()`. The returned path is the expected location regardless of whether the file actually exists -- use `keyFileExists()` to check.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Unlocker.keyFileExists$`
- `$API.Unlocker.writeKeyFile$`
- `$API.Unlocker.loadKeyFile$`

## getRegisteredMachineId

**Signature:** `String getRegisteredMachineId()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations on the returned string.
**Minimal Example:** `var machineId = {obj}.getRegisteredMachineId();`

**Description:**
Returns the machine ID extracted from the loaded license key file. The machine ID is parsed from the `"Machine numbers: "` line in the key file during `loadKeyFile()`. Returns an empty string if no key file has been loaded or if the key file does not contain a machine numbers line.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Unlocker.loadKeyFile$`
- `$API.Unlocker.getUserEmail$`

## getUserEmail

**Signature:** `String getUserEmail()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations on the returned string.
**Minimal Example:** `var email = {obj}.getUserEmail();`

**Description:**
Returns the email address associated with the loaded license key. This delegates to the JUCE `OnlineUnlockStatus` email extraction from the RSA-validated key file data. Returns an empty string if no valid key file has been loaded.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Unlocker.loadKeyFile$`
- `$API.Unlocker.getRegisteredMachineId$`

## isUnlocked

**Signature:** `bool isUnlocked()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var licensed = {obj}.isUnlocked();`

**Description:**
Returns whether the plugin is currently unlocked (licensed). This delegates to `juce::OnlineUnlockStatus::isUnlocked()`, which checks whether a valid RSA-signed key file has been loaded and validated. The result is also used internally by `CHECK_COPY_AND_RETURN_N` macros throughout HISE to gate audio processing in unlicensed builds.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Unlocker.loadKeyFile$`
- `$API.Unlocker.writeKeyFile$`
- `$API.Unlocker.contains$`

## isValidKeyFile

**Signature:** `bool isValidKeyFile(String possibleKeyData)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations.
**Minimal Example:** `var valid = {obj}.isValidKeyFile(keyDataString);`

**Description:**
Checks whether the given string looks like a valid JUCE RSA key file by verifying it starts with `"Keyfile for "`. This is a format check only -- it does not perform RSA signature validation. Use this to pre-validate user-provided key data before passing it to `writeKeyFile()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| possibleKeyData | String | no | Key file content to validate | Must start with `"Keyfile for "` to pass |

**Cross References:**
- `$API.Unlocker.writeKeyFile$`
- `$API.Unlocker.loadKeyFile$`

## keyFileExists

**Signature:** `bool keyFileExists()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** File system existence check performs I/O.
**Minimal Example:** `var exists = {obj}.keyFileExists();`

**Description:**
Returns whether the license key file exists on disk at the expected location. The file path is determined by `getLicenseKeyFile()`. Use this to decide whether to show a registration dialog or attempt to load an existing key.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Unlocker.getLicenseKeyFile$`
- `$API.Unlocker.loadKeyFile$`

## loadExpansionList

**Signature:** `bool loadExpansionList()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** File I/O, RSA decryption, BlowFish decryption, XML parsing.
**Minimal Example:** `var ok = {obj}.loadExpansionList();`

**Description:**
Loads and decrypts the expansion list file, then passes the decrypted credentials to `ExpansionHandler.setCredentials()` to unlock individual expansions. Requires `HISE_USE_UNLOCKER_FOR_EXPANSIONS` to be enabled -- throws a script error if the preprocessor is not set. Also requires the plugin to be unlocked (`isUnlocked()` must return true); returns false if not unlocked.

The decryption flow applies RSA public key decryption followed by BlowFish decryption (keyed with the registered machine ID), then validates the resulting XML payload against the current email, machine ID, and product name.

This method is called automatically by the constructor if the preprocessor is enabled and the plugin is unlocked.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Throws a script error (not a silent failure) if `HISE_USE_UNLOCKER_FOR_EXPANSIONS` is not enabled. This is a compile-time configuration issue, not a runtime error the user can recover from.

**Cross References:**
- `$API.Unlocker.writeExpansionKeyFile$`
- `$API.Unlocker.unlockExpansionList$`
- `$API.Unlocker.isUnlocked$`
- `$API.ExpansionHandler.setCredentials$`

## loadKeyFile

**Signature:** `bool loadKeyFile()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** File I/O, RSA validation, string parsing.
**Minimal Example:** `var ok = {obj}.loadKeyFile();`

**Description:**
Reads the license key file from disk and validates it using RSA signature verification. On success, the plugin becomes unlocked and the registered machine ID and user email are extractable via `getRegisteredMachineId()` and `getUserEmail()`. The key file path is determined by `getLicenseKeyFile()`.

This method is called automatically by the constructor if the key file exists on disk. Calling it manually is useful after `writeKeyFile()` to re-validate or after the key file has been updated externally.

The product ID matching uses the `doesProductIDMatch()` virtual, which by default strips version numbers (so a key for "MyPlugin 1.0" validates "MyPlugin 2.0"). Use `setProductCheckFunction()` to override this.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Unlocker.writeKeyFile$`
- `$API.Unlocker.isUnlocked$`
- `$API.Unlocker.getLicenseKeyFile$`
- `$API.Unlocker.setProductCheckFunction$`

## setProductCheckFunction

**Signature:** `void setProductCheckFunction(Function checkFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** WeakCallbackHolder construction allocates.
**Minimal Example:** `{obj}.setProductCheckFunction(onProductCheck);`

**Description:**
Sets a custom callback function that overrides the default product ID matching logic used during key file validation. By default, HISE strips version numbers from the product ID comparison, so a key file for "MyPlugin 1.0" also unlocks "MyPlugin 2.0". Use this method to implement version-aware or custom product ID matching.

The callback is stored as a `WeakCallbackHolder` and invoked synchronously during `loadKeyFile()` when the RSA-decrypted key data is validated. The callback receives the product ID string from the key file and must return a boolean indicating whether the product matches.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| checkFunction | Function | no | Custom product ID matching callback | Must accept 1 argument, must return bool |

**Callback Signature:** checkFunction(returnedIDFromServer: String)

**Pitfalls:**
- The callback is called synchronously during `loadKeyFile()` via `callSync`. If the callback throws an error or returns a non-boolean value, the product check falls through and the key file validation fails silently.

**Cross References:**
- `$API.Unlocker.loadKeyFile$`
- `$API.Unlocker.isUnlocked$`

**Example:**
```javascript:version-aware-check
// Title: Version-aware product ID matching
const var ul = Engine.createLicenseUnlocker();

inline function onProductCheck(returnedId)
{
    // Require exact product name + version match
    local expectedId = Engine.getName() + " " + Engine.getVersion();
    return returnedId == expectedId;
}

ul.setProductCheckFunction(onProductCheck);
```
```json:testMetadata:version-aware-check
{
  "testable": false,
  "skipReason": "Requires a valid RSA key file and copy protection configuration"
}
```

## unlockExpansionList

**Signature:** `bool unlockExpansionList(var expansionIdList)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** File I/O scanning expansion folders, XML parsing, string operations.
**Minimal Example:** `var ok = {obj}.unlockExpansionList(expansionIds);`

**Description:**
Backend-only method that scans expansion folders for metadata, collects encryption keys, and passes them to `ExpansionHandler.setCredentials()`. This is used during development to unlock expansions without a signed expansion list file. In frontend builds, this method returns false without performing any work.

The method iterates over expansion folders, reading either `project_info.xml` (for `FullInstrumentExpansion` types) or `expansion_info.xml` (for standard expansions) to extract encryption keys.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| expansionIdList | NotUndefined | no | Expansion identifiers to unlock | -- |

**Pitfalls:**
- Silently returns false in frontend builds with no error message or warning. Code that relies on this method will appear to work in the HISE IDE but silently fail in exported plugins.

**Cross References:**
- `$API.Unlocker.loadExpansionList$`
- `$API.Unlocker.writeExpansionKeyFile$`
- `$API.ExpansionHandler.setCredentials$`

## writeExpansionKeyFile

**Signature:** `bool writeExpansionKeyFile(String keyData)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** File I/O write, then delegates to loadExpansionList() for decryption.
**Minimal Example:** `var ok = {obj}.writeExpansionKeyFile(expansionKeyData);`

**Description:**
Writes expansion key data to the expansion list file (a sibling of the main license key file named "expansions" with the same extension). Requires `HISE_USE_UNLOCKER_FOR_EXPANSIONS` to be enabled -- throws a script error if the preprocessor is not set. The `keyData` string must start with `"Expansion List"` or the method silently returns false. On successful write, automatically calls `loadExpansionList()` to decrypt and apply the expansion credentials.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| keyData | String | no | Expansion list key file content from server | Must start with `"Expansion List"` |

**Pitfalls:**
- [BUG] Silently returns false if `keyData` does not start with `"Expansion List"`. No error message is produced, making it difficult to diagnose malformed expansion key data.

**Cross References:**
- `$API.Unlocker.loadExpansionList$`
- `$API.Unlocker.unlockExpansionList$`
- `$API.Unlocker.getLicenseKeyFile$`
- `$API.Unlocker.writeKeyFile$`

## writeKeyFile

**Signature:** `bool writeKeyFile(String keyData)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** File I/O write to the license key file location.
**Minimal Example:** `var ok = {obj}.writeKeyFile(keyFileContent);`

**Description:**
Writes the given key file content to the license key file location on disk. The file path is determined by `getLicenseKeyFile()`. This does not validate or load the key -- call `loadKeyFile()` afterwards to validate the RSA signature and unlock the plugin. Use `isValidKeyFile()` to pre-validate the data format before writing.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| keyData | String | no | Key file content to write to disk | Should start with `"Keyfile for "` for validity |

**Cross References:**
- `$API.Unlocker.loadKeyFile$`
- `$API.Unlocker.isValidKeyFile$`
- `$API.Unlocker.getLicenseKeyFile$`
- `$API.Unlocker.writeExpansionKeyFile$`
