## setFrontendMacros

**Examples:**

```javascript:generated-automation-slots
// Title: Setting up automation slots with generated names
// Context: Plugins with many automatable parameters generate macro
// names programmatically rather than hardcoding them. This pattern
// creates a numbered list of automation slot names and enables the
// macro system at initialization.

const var NUM_SLOTS = 8;

inline function initAutomationSlots()
{
    local slotIds = [];
    slotIds.reserve(NUM_SLOTS);

    for (i = 0; i < NUM_SLOTS; i++)
    {
        // Zero-pad single digits for consistent display
        local label = "Automation " + (i < 9 ? "0" : "") + (i + 1);
        slotIds.push(label);
    }

    Engine.setFrontendMacros(slotIds);
}

initAutomationSlots();
```
```json:testMetadata:generated-automation-slots
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Engine.getMacroName(1)", "value": "Automation 01"},
    {"type": "REPL", "expression": "Engine.getMacroName(8)", "value": "Automation 08"}
  ]
}
```

```javascript:named-modulation-macros
// Title: Named modulation source macros
// Context: Synthesizers with a fixed set of modulation sources use
// descriptive macro names that map to the modulation architecture.

Engine.setFrontendMacros([
    "LFO1", "LFO2", "ENV1", "ENV2", "VELO", "NOTE"
]);
```
```json:testMetadata:named-modulation-macros
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Engine.getMacroName(1)", "value": "LFO1"},
    {"type": "REPL", "expression": "Engine.getMacroName(6)", "value": "NOTE"}
  ]
}
```
