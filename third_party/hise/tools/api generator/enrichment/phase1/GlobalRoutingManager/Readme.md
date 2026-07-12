# GlobalRoutingManager -- Class Analysis

## Brief
Singleton factory for named GlobalCable instances with OSC send/receive and per-event data storage.

## Purpose
GlobalRoutingManager is the central access point for the global routing system. It creates and retrieves GlobalCable instances by name, manages bidirectional OSC connections for external controller communication, provides script-level OSC message callbacks, and maintains a per-event-ID data storage that can be read by modulators and scriptnode nodes. It is a singleton per MainController, lazily created on first access.

## Details

### Architecture

GlobalRoutingManager is a scripting wrapper around `scriptnode::routing::GlobalRoutingManager`, a `ReferenceCountedObject` singleton stored on the `MainController`. Each call to `Engine.getGlobalRoutingManager()` creates a new wrapper, but all wrappers share the same underlying singleton. The manager owns two slot lists (cables and signals), though only cables are exposed to the scripting API.

### OSC Connection Model

The `connectToOSC` method configures a bidirectional OSC connection with domain prefix, port binding, and per-cable value normalisation ranges. See `connectToOSC()` for the full configuration object schema and `addOSCCallback()` for script-level message handling.

Setting `TargetPort` to `-1` (or omitting it) creates a receive-only connection. When a target port is specified, each cable whose ID starts with `/` automatically gets an OSC output target that sends value changes as OSC messages. The `Parameters` property maps cable IDs to range definitions for bidirectional value normalisation.

### OSC-Addressable Cable Convention

For cables to participate in OSC routing, their IDs must start with `/`. When an OSC message arrives, the domain prefix is stripped and the remaining address is matched against cable IDs. Multi-argument OSC messages map arguments to cables with bracketed indices (e.g., `/param[0]`, `/param[1]`).

### Event Data Storage

The manager contains a fixed-size hash table (1024 event slots x 16 data slots per event) for attaching arbitrary numeric data to MIDI events. Stored values are accessible downstream by EventDataModulator, scriptnode routing nodes, and the ComplexGroupManager. See `setEventData()` and `getEventData()` for the storage/retrieval API and hash collision behavior.

## obtainedVia
`Engine.getGlobalRoutingManager()`

## minimalObjectToken
rm

## Constants
(None)

## Dynamic Constants
(None)

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Using cable IDs without `/` prefix and expecting OSC routing | Start cable IDs with `/` (e.g., `"/volume"`) | Only cables whose IDs begin with `/` participate in OSC send/receive. Non-prefixed cables work for internal routing but are invisible to OSC. |
| Calling `sendOSCMessage` without first calling `connectToOSC` with a `TargetPort` | Call `connectToOSC` with a valid `TargetPort` before sending | Without a target port, no sender is created and `sendOSCMessage` silently returns false. |

## codeExample
```javascript
// Set up global routing manager with OSC
const var rm = Engine.getGlobalRoutingManager();

rm.connectToOSC({
    "Domain": "/myPlugin",
    "SourcePort": 9000,
    "TargetPort": 9001
}, function(error) { Console.print("OSC Error: " + error); });

const var cable = rm.getCable("/volume");
```

## Alternatives
GlobalCable -- the individual named value bus created by this manager. Broadcaster -- for scripting-only pub/sub without global routing infrastructure or OSC.

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: All methods either create objects on demand (getCable) or return clear success/failure booleans. OSC errors are routed through the error callback. No silent-failure preconditions that would benefit from parse-time diagnostics.
