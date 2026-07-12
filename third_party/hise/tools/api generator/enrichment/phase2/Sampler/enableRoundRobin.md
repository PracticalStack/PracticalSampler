## enableRoundRobin

**Examples:**

```javascript:disable-rr-init
// Title: Disable automatic round-robin for manual group control
// Context: When a sampler needs context-dependent group selection (e.g., short vs.
// long release samples), disable automatic RR at init and use setActiveGroup() later.

// In onInit - disable RR for all child samplers
const var samplerNames = Synth.getIdList("Sampler");
const var samplers = [];

for (name in samplerNames)
{
    samplers.push(Synth.getSampler(name));
    samplers[samplers.length - 1].enableRoundRobin(false);
}

// Now setActiveGroup() can be used in onNoteOn
```

```json:testMetadata:disable-rr-init
{
  "testable": false,
  "skipReason": "Requires sampler module tree"
}
```

This is the required first step before any manual group management. All group-control methods (`setActiveGroup`, `setMultiGroupIndex`, `getRRGroupsForMessage`) will throw a script error if round-robin is still enabled.
