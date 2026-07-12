# ExpansionHandler -- Method Documentation

## encodeWithCredentials

**Signature:** `bool encodeWithCredentials(var hxiFile)`
**Return Type:** `bool`
**Call Scope:** unsafe
**Call Scope Note:** File I/O and BlowFish encryption operations.
**Minimal Example:** `var ok = {obj}.encodeWithCredentials(myHxiFile);`

**Description:**
Encrypts an intermediate `.hxi` file into a credential-encrypted `.hxp` file using the credentials previously set via `setCredentials()`. The argument must be a File object pointing to an existing `.hxi` file. Returns true on success.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| hxiFile | ScriptObject | no | File object pointing to an .hxi file | Must be a File object; non-File triggers script error |

**Pitfalls:**
- Credentials must be set via `setCredentials()` before calling this method. If no credentials are configured, the encryption may fail or produce an invalid `.hxp` file.

**Cross References:**
- `$API.ExpansionHandler.setCredentials$`

## getCurrentExpansion

**Signature:** `var getCurrentExpansion()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a ScriptExpansionReference wrapper on the heap.
**Minimal Example:** `var e = {obj}.getCurrentExpansion();`

**Description:**
Returns a reference to the currently active expansion pack, or `undefined` if no expansion is active. The active expansion is set via `setCurrentExpansion()`.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.ExpansionHandler.setCurrentExpansion$`
- `$API.Expansion$`

## getExpansion

**Signature:** `var getExpansion(var name)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a ScriptExpansionReference wrapper on the heap.
**Minimal Example:** `var e = {obj}.getExpansion("MyExpansion");`

**Description:**
Returns a reference to the expansion with the given name, or `undefined` if no expansion with that name is found. Only searches successfully initialized expansions.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| name | String | no | The name of the expansion to find | Matched against the expansion's Name property |

**Cross References:**
- `$API.ExpansionHandler.getExpansionList$`
- `$API.Expansion$`

## getExpansionForInstallPackage

**Signature:** `var getExpansionForInstallPackage(var packageFile)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** File I/O to read archive metadata, heap allocation for wrapper.
**Minimal Example:** `var e = {obj}.getExpansionForInstallPackage(myPackageFile);`

**Description:**
Checks whether an expansion from the given `.hr` package file is already installed. Reads the package metadata to determine the target expansion folder, then searches for an existing expansion at that location. Returns the expansion reference if found, `undefined` otherwise. FileBased expansions are deliberately excluded from detection to simulate the end-user installation flow where only encoded/encrypted expansions represent installed packages.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| packageFile | ScriptObject | no | File object pointing to an .hr package | Must be a File object; non-File triggers script error |

**Pitfalls:**
- FileBased expansions at the target location are ignored. This method only detects Intermediate (.hxi) and Encrypted (.hxp) expansions.
- If the package metadata cannot be read, logs a warning via the error function but does not throw a script error.

**Cross References:**
- `$API.ExpansionHandler.installExpansionFromPackage$`
- `$API.ExpansionHandler.getMetaDataFromPackage$`
- `$API.Expansion$`

## getExpansionList

**Signature:** `var getExpansionList()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates ScriptExpansionReference wrappers for each expansion.
**Minimal Example:** `var list = {obj}.getExpansionList();`

**Description:**
Returns an array of Expansion references for all successfully initialized expansion packs. Expansions that failed initialization (missing credentials, disallowed type) are excluded -- use `getUninitialisedExpansions()` for those. The list is sorted alphabetically by expansion name.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.ExpansionHandler.getExpansion$`
- `$API.ExpansionHandler.setAllowedExpansionTypes$`
- `$API.Expansion$`

## getMetaDataFromPackage

**Signature:** `var getMetaDataFromPackage(var packageFile)`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** File I/O to read HLAC archive metadata.
**Minimal Example:** `var meta = {obj}.getMetaDataFromPackage(myPackageFile);`

**Description:**
Reads metadata directly from an `.hr` archive package file without installing it. Returns a JSON object containing package metadata (including the `HxiName` property identifying the target expansion). Useful for displaying package information before installation.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| packageFile | ScriptObject | no | File object pointing to an .hr package | Must be a File object |

**Pitfalls:**
- [BUG] If the argument is not a File object, the method silently returns `undefined` without reporting an error. Other similar methods (`encodeWithCredentials`, `getExpansionForInstallPackage`) correctly report a script error for non-File arguments.

**Cross References:**
- `$API.ExpansionHandler.getExpansionForInstallPackage$`
- `$API.ExpansionHandler.installExpansionFromPackage$`

## getUninitialisedExpansions

**Disabled:** no-op
**Disabled Reason:** Method is declared and implemented in C++ but not registered in the constructor via `ADD_API_METHOD_0`. It cannot be called from HiseScript. See issues.md for the registration bug.

## installExpansionFromPackage

**Signature:** `bool installExpansionFromPackage(var packageFile, var sampleDirectory)`
**Return Type:** `bool`
**Call Scope:** unsafe
**Call Scope Note:** Kills voices via killVoicesAndCall, runs on SampleLoadingThread, performs file I/O and HLAC decompression.
**Minimal Example:** `var ok = {obj}.installExpansionFromPackage(myPackageFile, FileSystem.Samples);`

**Description:**
Installs an expansion from an `.hr` archive package file. The installation runs on the SampleLoadingThread (voices are killed first). The method decompresses the archive into the expansion directory, optionally encrypts the intermediate file if credentials are set, then reinitialises all expansions. The `sampleDirectory` parameter accepts either a `FileSystem` location constant (`FileSystem.Expansions` or `FileSystem.Samples`) or a File object pointing to a custom sample directory.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| packageFile | ScriptObject | no | File object pointing to an .hr package | Must be a File object; non-File triggers script error |
| sampleDirectory | Integer or ScriptObject | no | Target directory for samples | FileSystem.Expansions, FileSystem.Samples, or a File object pointing to an existing directory |

**Pitfalls:**
- If `sampleDirectory` is neither a recognized FileSystem constant nor a File object, `targetFolder` remains unset and the method reports "The sample directory does not exist".
- If credentials are set via `setCredentials()`, the intermediate file is automatically encrypted to `.hxp`. Otherwise it is saved as `.hxi`. This happens silently based on credential state.

**Cross References:**
- `$API.ExpansionHandler.setInstallCallback$`
- `$API.ExpansionHandler.setCredentials$`
- `$API.ExpansionHandler.getExpansionForInstallPackage$`

**Example:**
```javascript:install-expansion-with-progress
// Title: Install expansion with progress tracking
const var eh = Engine.createExpansionHandler();

inline function onInstallProgress(state)
{
    if (state.Status == 0)
        Console.print("Installation started");
    else if (state.Status == 1)
        Console.print("Progress: " + Math.round(state.TotalProgress * 100) + "%");
    else if (state.Status == 2)
    {
        Console.print("Installation complete");

        if (isDefined(state.Expansion))
            Console.print("Installed: " + state.Expansion.getProperties().Name);
    }
};

eh.setInstallCallback(onInstallProgress);
eh.installExpansionFromPackage(myPackageFile, FileSystem.Samples);
```
```json:testMetadata:install-expansion-with-progress
{
  "testable": false,
  "skipReason": "Requires an .hr archive package file that cannot be created via script API"
}
```

## refreshExpansions

**Signature:** `bool refreshExpansions()`
**Return Type:** `bool`
**Call Scope:** unsafe
**Call Scope Note:** Filesystem scan, heap allocations for new expansion objects.
**Minimal Example:** `var ok = {obj}.refreshExpansions();`

**Description:**
Rescans the expansion folder for new or changed expansions. Discovers any new expansion directories, creates expansion objects for them, and sorts the list alphabetically. Returns true on success. Triggers the expansion callback if new expansions are found.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.ExpansionHandler.getExpansionList$`
- `$API.ExpansionHandler.setExpansionCallback$`

## setAllowedExpansionTypes

**Signature:** `void setAllowedExpansionTypes(var typeList)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Modifies expansion handler state, may trigger reinitialization of disallowed expansions.
**Minimal Example:** `{obj}.setAllowedExpansionTypes([{obj}.Intermediate, {obj}.Encrypted]);`

**Description:**
Restricts which expansion types can be loaded. Takes an array of expansion type constants (`FileBased`, `Intermediate`, `Encrypted`). Expansions whose type is not in the allowed list are moved to the uninitialised list and become invisible to `getExpansionList()`. This filtering is applied during expansion discovery, reinitialization, and when rebuilding uninitialised expansions.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| typeList | Array | no | Array of expansion type constants | Must be an array; non-array triggers script error |

**Cross References:**
- `$API.ExpansionHandler.getExpansionList$`
- `$API.ExpansionHandler.refreshExpansions$`

## setCredentials

**Signature:** `void setCredentials(var newCredentials)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Stores a var (ref-counted copy) in the core ExpansionHandler.
**Minimal Example:** `{obj}.setCredentials({"key": "myLicenseKey"});`

**Description:**
Sets credentials for decrypting `.hxp` encrypted expansion packs. The argument must be a JSON object. If a non-object is passed, the method calls the error function with the message "credentials must be an object" rather than throwing a script error, so execution continues.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newCredentials | JSON | no | Credentials object for expansion decryption | Must be a DynamicObject; non-object triggers error callback, not script error |

**Pitfalls:**
- Passing a non-object (string, number, array) does not throw a script error. Instead, it sends a message via the error function callback. If no error function is set, the invalid input is silently ignored and credentials remain unchanged.

**Cross References:**
- `$API.ExpansionHandler.setErrorFunction$`
- `$API.ExpansionHandler.encodeWithCredentials$`
- `$API.ExpansionHandler.installExpansionFromPackage$`

## setCurrentExpansion

**Signature:** `bool setCurrentExpansion(var expansionName)`
**Return Type:** `bool`
**Call Scope:** unsafe
**Call Scope Note:** State change with async notifications, potential preset save/restore via FullInstrumentExpansion.
**Minimal Example:** `var ok = {obj}.setCurrentExpansion("MyExpansion");`

**Description:**
Sets the currently active expansion by name or by Expansion object reference. Triggers the expansion callback with the new expansion. If this is the first expansion activation (transitioning from no expansion), saves the current default state for later restoration. Checks HISE version compatibility between the expansion and the current build. Pass an empty string to clear the current expansion (the callback receives `undefined`).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| expansionName | String | no | Expansion name or Expansion reference | Accepts String (expansion name) or ScriptObject (Expansion reference); empty string clears current expansion |

**Pitfalls:**
- If the argument is neither a String nor an Expansion reference, the method reports "can't find expansion" as a script error rather than indicating the type mismatch.

**Cross References:**
- `$API.ExpansionHandler.getCurrentExpansion$`
- `$API.ExpansionHandler.setExpansionCallback$`
- `$API.ExpansionHandler.getExpansion$`

## setEncryptionKey

**Disabled:** deprecated
**Disabled Reason:** Always throws a script error: "This function is deprecated. Use the project settings to setup the project's blowfish key". The BlowFish encryption key is now configured in the HISE Project Settings, not via script.

## setErrorFunction

**Signature:** `void setErrorFunction(var newErrorFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a WeakCallbackHolder on the heap.
**Minimal Example:** `{obj}.setErrorFunction(onExpansionError);`

**Description:**
Sets a callback function that receives expansion-related error and log messages. The callback receives two arguments: a message string and a boolean indicating whether the error is critical. Messages are triggered by expansion initialization failures, credential validation errors, and manual calls to `setErrorMessage()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newErrorFunction | Function | yes | Callback for error messages | Must be a function |

**Callback Signature:** newErrorFunction(message: String, isCritical: bool)

**Pitfalls:**
- [BUG] The WeakCallbackHolder is created with `numExpectedArgs=1` but `logMessage()` passes 2 arguments. The callback still receives both arguments at runtime, but the parse-time diagnostic metadata is inconsistent. See issues.md.

**Cross References:**
- `$API.ExpansionHandler.setErrorMessage$`

**Example:**
```javascript:error-function-with-severity
// Title: Error function with severity routing
const var eh = Engine.createExpansionHandler();

inline function onExpansionError(message, isCritical)
{
    if (isCritical)
        Console.print("CRITICAL: " + message);
    else
        Console.print("Info: " + message);
};

eh.setErrorFunction(onExpansionError);

// --- test-only ---
eh.setErrorMessage("test warning");
// --- end test-only ---
```
```json:testMetadata:error-function-with-severity
{
  "testable": true,
  "verifyScript": {"type": "log-output", "values": ["Info: test warning"]}
}
```

## setErrorMessage

**Signature:** `void setErrorMessage(var errorMessage)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Dispatches the error callback via WeakCallbackHolder.
**Minimal Example:** `{obj}.setErrorMessage("Custom warning message");`

**Description:**
Manually triggers the error function callback with the given message and `isCritical` set to `false`. Useful for injecting custom warning messages into the expansion error handling pipeline.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| errorMessage | String | no | The error message to send | Converted to String via toString() |

**Pitfalls:**
- If no error function has been set via `setErrorFunction()`, the message is silently discarded.

**Cross References:**
- `$API.ExpansionHandler.setErrorFunction$`

## setExpansionCallback

**Signature:** `void setExpansionCallback(var expansionLoadedCallback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a WeakCallbackHolder on the heap, increments ref count, registers as source.
**Minimal Example:** `{obj}.setExpansionCallback(onExpansionLoaded);`

**Description:**
Sets a callback that fires when an expansion is loaded, created, or cleared. The callback receives a single argument: an Expansion object reference when an expansion is activated or discovered, or `undefined` when the current expansion is cleared. Both `expansionPackLoaded` and `expansionPackCreated` listener events trigger this callback.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| expansionLoadedCallback | Function | yes | Callback for expansion lifecycle events | Must be a function |

**Callback Signature:** expansionLoadedCallback(expansion: var)

**Cross References:**
- `$API.ExpansionHandler.setCurrentExpansion$`
- `$API.ExpansionHandler.refreshExpansions$`
- `$API.Expansion$`

**Example:**
```javascript:expansion-callback-handling
// Title: Expansion load callback with null check
const var eh = Engine.createExpansionHandler();

inline function onExpansionLoaded(e)
{
    if (isDefined(e))
        Console.print("Loaded: " + e.getProperties().Name);
    else
        Console.print("No expansion active");
};

eh.setExpansionCallback(onExpansionLoaded);

// --- test-only ---
eh.setCurrentExpansion("");
// --- end test-only ---
```
```json:testMetadata:expansion-callback-handling
{
  "testable": true,
  "verifyScript": {"type": "log-output", "values": ["No expansion active"]}
}
```

## setInstallCallback

**Signature:** `void setInstallCallback(var installationCallback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a WeakCallbackHolder on the heap, increments ref count, registers as source.
**Minimal Example:** `{obj}.setInstallCallback(onInstallProgress);`

**Description:**
Sets a callback for tracking expansion installation progress. The callback receives a JSON object with installation status properties. It fires at three points: when installation starts (Status 0), periodically during installation at 300ms intervals (Status 1), and when installation completes (Status 2). Must be set before calling `installExpansionFromPackage()` for progress tracking to work.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| installationCallback | Function | yes | Callback for installation progress | Must be a function |

**Callback Signature:** installationCallback(state: Object)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| Status | Integer | -1 = not started, 0 = started, 1 = in progress, 2 = complete |
| Progress | Double | Sample preload progress (0.0-1.0) |
| TotalProgress | Double | Overall installation progress (0.0-1.0) |
| SourceFile | ScriptObject | File object of the .hr package being installed |
| TargetFolder | ScriptObject | File object of the expansion root directory |
| SampleFolder | ScriptObject | File object of the sample destination directory |
| Expansion | ScriptObject | The created Expansion reference (only valid when Status is 2, undefined otherwise) |

**Cross References:**
- `$API.ExpansionHandler.installExpansionFromPackage$`
- `$API.Expansion$`

## setInstallFullDynamics

**Signature:** `void setInstallFullDynamics(var shouldInstallFullDynamics)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Simple boolean assignment on the core ExpansionHandler.
**Minimal Example:** `{obj}.setInstallFullDynamics(true);`

**Description:**
Controls whether full dynamic range samples are extracted during expansion installation. When set to true, the installer extracts the full (uncompressed) sample data instead of the HLAC-compressed version. This setting must be configured before calling `installExpansionFromPackage()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldInstallFullDynamics | Integer | no | Whether to extract full dynamic samples | Boolean value (0 or 1) |

**Cross References:**
- `$API.ExpansionHandler.installExpansionFromPackage$`
