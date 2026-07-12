# DisplayBufferSource -- Class Analysis

## Brief
Handle to a processor's display buffer slots for real-time visualization access.

## Purpose
DisplayBufferSource is a lightweight wrapper around a processor that owns one or more display buffers (ring buffers used for waveform/spectrum visualization). It is obtained via `Synth.getDisplayBufferSource()` by passing a processor ID, and its sole method `getDisplayBuffer()` retrieves individual `DisplayBuffer` objects by index. The class acts as an intermediary in the two-step access pattern: locate the processor first, then access its display buffer slots.

## obtainedVia
`Synth.getDisplayBufferSource(processorId)`

## minimalObjectToken
dbs

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `var dbs = Synth.getDisplayBufferSource("MyEffect"); // in onControl` | `const var dbs = Synth.getDisplayBufferSource("MyEffect"); // in onInit` | Must be called in onInit -- the factory method checks objectsCanBeCreated() and throws an error outside initialization. |
| `dbs.getDisplayBuffer(5); // processor only has 1 buffer` | `dbs.getDisplayBuffer(0);` | The index must be within the number of display buffers the processor owns. Out-of-range indices cause a script error. |

## codeExample
```javascript
// Get a display buffer source from a processor and access its first buffer
const var dbs = Synth.getDisplayBufferSource("MyProcessor");
const var db = dbs.getDisplayBuffer(0);
```

## Alternatives
DisplayBuffer provides the actual buffer data and visualization methods. DisplayBufferSource is only the accessor for retrieving DisplayBuffer objects from a named processor.

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: This class has a single method with straightforward index validation and no silent-failure preconditions beyond the onInit lifecycle constraint already enforced by the factory method.
