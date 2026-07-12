<!-- Diagram triage:
  - No class-level diagrams specified in Phase 1
-->

# GlobalRoutingManager

GlobalRoutingManager is the central access point for the global routing system - a project-wide message bus that sends normalised float values between script, scriptnode DSP networks, and external OSC controllers through named cables.

The system addresses three main use cases:

1. **DSP-to-script bridging** - read internal DSP network state (levels, envelope positions, parameter feedback) back into script for UI visualisation, without Broadcasters.
2. **Script-to-DSP parameter control** - send UI slider values into DSP network parameters through named cables, with optional range conversion.
3. **External OSC communication** - connect to OSC controllers for bidirectional parameter mapping, with per-cable value normalisation and script-level message callbacks.

```javascript
const var rm = Engine.getGlobalRoutingManager();
const var cable = rm.getCable("/volume");
```

Cables carry normalised double-precision values (0.0 to 1.0) by default, but the scripting API on [GlobalCable]($API.GlobalCable$) provides built-in range scaling, skew, and stepping. Cables whose IDs start with `/` are automatically OSC-addressable when an OSC connection is active. All other cables work for internal routing only.

A separate subsystem provides per-event data storage: up to 16 double-precision slots per MIDI event, accessible from script, voice-start modulators, and scriptnode nodes. This is independent of the cable system.

> GlobalRoutingManager is a singleton - all calls to `Engine.getGlobalRoutingManager()` share the same underlying instance. Store the reference in a `const var` and reuse it. All cables show up in the HISE IDE module tree with navigation to each connection point.

## Common Mistakes

- **Start cable IDs with `/` for OSC routing**
  **Wrong:** Using cable IDs without `/` prefix and expecting OSC routing
  **Right:** Start cable IDs with `/` (e.g., `"/volume"`)
  *Only cables whose IDs begin with `/` participate in OSC send/receive. Non-prefixed cables work for internal routing but are invisible to OSC.*

- **Match cable names exactly between script and DSP network**
  **Wrong:** Using different cable name strings in script vs. DSP network
  **Right:** Ensure the cable ID in `getCable("name")` exactly matches the scriptnode `processorId`
  *Cable names are matched by exact string equality. A mismatch silently creates a disconnected cable with no error.*

- **Store the manager reference once**
  **Wrong:** Creating a separate `Engine.getGlobalRoutingManager()` call for each cable
  **Right:** Call `Engine.getGlobalRoutingManager()` once and reuse the reference
  *Each call creates a new wrapper object. Store the manager in a `const var` and call `getCable` multiple times on it.*

- **Configure a TargetPort before sending OSC**
  **Wrong:** Calling `sendOSCMessage` without first calling `connectToOSC` with a `TargetPort`
  **Right:** Call `connectToOSC` with a valid `TargetPort` before sending
  *Without a target port, no sender is created and `sendOSCMessage` silently returns false.*
