# BeatportManager -- Class Analysis

## Brief
Beatport SDK DRM integration service for product access validation and licensing.

## Purpose
BeatportManager wraps the Beatport authentication system for DRM validation in HISE plugins distributed through the Beatport platform. It provides methods to set a product ID, validate access against the Beatport SDK, and check whether the current session has valid Beatport access. The entire SDK integration is gated behind the `HISE_INCLUDE_BEATPORT` preprocessor flag (default: off). When the flag is disabled, the class operates in simulation mode using a local JSON file for development and testing.

## Details

### SDK Integration Architecture

BeatportManager uses a double-indirection pimpl pattern to isolate the Beatport SDK:
- `BeatportManager` holds a `ScopedPointer<Pimpl>`
- `Pimpl` holds a raw `Data*` pointer to an opaque struct

This ensures Beatport SDK headers never leak into HISE's compilation units. The `Pimpl` and its `Data` struct are only compiled when `HISE_INCLUDE_BEATPORT=1`.

### Simulation Mode (Default)

When `HISE_INCLUDE_BEATPORT=0` (the default), all methods operate as development stubs with simulated delays that mimic real SDK latency. See `setProductId()`, `validate()`, and `isBeatportAccess()` for per-method simulation details. The `validate_response.json` file at `{project}/AdditionalSourceCode/beatport/` must be created manually by the developer with the expected JSON response structure.

### Blocking Calls and Timeout Extension

Both `validate()` and `isBeatportAccess()` are synchronous blocking calls. After execution, they call `extendTimeout()` on the script engine to prevent the script watchdog from killing the script during the wait period. This timeout extension covers both simulation waits and real SDK calls.

### getBeatportProjectFolder (C++ Only)

The static method `getBeatportProjectFolder` is a C++ utility that returns `{project}/AdditionalSourceCode/beatport/`. It is backend-only (`USE_BACKEND` guard) and is NOT exposed to HiseScript despite appearing in the Doxygen output. It is used internally by the simulation code paths.

## obtainedVia
`Engine.createBeatportManager()`

## minimalObjectToken
bp

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes
| Wrong | Right | Explanation |
|---|---|---|
| Calling `validate()` without creating `validate_response.json` in simulation mode | Create `{project}/AdditionalSourceCode/beatport/validate_response.json` before calling `validate()` | Without the SDK enabled, `validate()` reads this file and throws a script error if it is missing. |

## codeExample
```javascript
const bp = Engine.createBeatportManager();
bp.setProductId("my-product-id");

if (bp.isBeatportAccess())
{
    var result = bp.validate();
    Console.print(trace(result));
}
```

## Alternatives
- `Unlocker` -- Both handle product DRM/licensing; BeatportManager uses the Beatport SDK while Unlocker uses RSA key file validation.

## Related Preprocessors
`HISE_INCLUDE_BEATPORT`, `USE_BACKEND`

## Diagrams
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: All three methods have straightforward contracts with no silent-failure preconditions or timeline dependencies that could benefit from parse-time diagnostics.
