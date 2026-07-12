## isBeatportAccess

**Signature:** `bool isBeatportAccess()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Blocks the calling thread (500ms simulated delay in development mode, SDK call in production). Extends the script engine timeout to prevent watchdog termination.
**Minimal Example:** `var access = {obj}.isBeatportAccess();`

**Description:**
Returns whether the current session has valid Beatport access. When `HISE_INCLUDE_BEATPORT` is enabled, delegates to the Beatport SDK. In simulation mode (default), waits 500ms to mimic SDK latency and returns `true` if `validate_response.json` exists in the project's `AdditionalSourceCode/beatport/` folder.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.BeatportManager.validate$`
- `$API.BeatportManager.setProductId$`
- `$API.Engine.createBeatportManager$`

## setProductId

**Signature:** `void setProductId(String productId)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** String construction and console logging in simulation mode; SDK call in production mode.
**Minimal Example:** `{obj}.setProductId("my-product-id");`

**Description:**
Sets the Beatport product identifier for this manager instance. When `HISE_INCLUDE_BEATPORT` is enabled, passes the ID to the Beatport SDK. In simulation mode (default), logs the product ID to the HISE console via `debugToConsole`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| productId | String | no | The Beatport product identifier string. | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.BeatportManager.validate$`
- `$API.BeatportManager.isBeatportAccess$`
- `$API.Engine.createBeatportManager$`

## validate

**Signature:** `var validate()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Blocks the calling thread (1.5s simulated delay in development mode, SDK call in production). Performs file I/O and JSON parsing in simulation mode. Extends the script engine timeout to prevent watchdog termination.
**Minimal Example:** `var result = {obj}.validate();`

**Description:**
Validates the current Beatport access and returns the result as a JSON object. When `HISE_INCLUDE_BEATPORT` is enabled, delegates to the Beatport SDK. In simulation mode (default), waits 1.5s to mimic network latency, then reads and parses `{project}/AdditionalSourceCode/beatport/validate_response.json`. Throws a script error if the file is missing or contains invalid JSON. The returned object structure depends on the Beatport SDK response format.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.BeatportManager.isBeatportAccess$`
- `$API.BeatportManager.setProductId$`
- `$API.Engine.createBeatportManager$`

## getBeatportProjectFolder

**Disabled:** no-op
**Disabled Reason:** Static C++ utility method (`static File getBeatportProjectFolder(MainController*)`) used internally by the simulation code paths. Not registered as a scripting API method and not callable from HiseScript. Backend-only (`USE_BACKEND` guard).
