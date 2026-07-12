<!-- Diagram triage:
  - No diagrams specified in Phase 1 data
-->

# BeatportManager

BeatportManager handles DRM validation for plugins distributed through the Beatport platform. It wraps the Beatport SDK behind a simple three-method interface: set a product identifier, check whether the session has Beatport access, and validate that access to receive a detailed response.

```js
const bp = Engine.createBeatportManager();
```

The typical validation workflow is:

1. Call `setProductId()` with your Beatport product identifier.
2. Check `isBeatportAccess()` to determine whether the session has Beatport access.
3. Call `validate()` to perform the full validation and receive the SDK response as a JSON object.

The entire Beatport integration is gated behind the `HISE_INCLUDE_BEATPORT` preprocessor flag, which is off by default. When the flag is disabled, all methods run in **simulation mode**: they introduce artificial delays to mimic SDK latency and use a local JSON file for validation responses. This allows you to develop and test the Beatport integration workflow without the SDK installed.

| Mode | `HISE_INCLUDE_BEATPORT` | Behaviour |
|---|---|---|
| Simulation (default) | Off | Artificial delays, reads `validate_response.json` from the project folder |
| Production | On | Delegates to the real Beatport SDK |

> Both `validate()` and `isBeatportAccess()` are synchronous blocking calls. They automatically extend the script engine timeout to prevent the watchdog from terminating the script during the wait.

## Common Mistakes

- **Create the simulation response file before calling validate()**
  **Wrong:** Calling `bp.validate()` without a response file in place
  **Right:** Create `{project}/AdditionalSourceCode/beatport/validate_response.json` with valid JSON before calling `validate()`
  *In simulation mode, `validate()` reads this file and throws a script error if it is missing or contains invalid JSON.*
