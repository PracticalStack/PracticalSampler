## extendTimeOut

**Examples:**

```javascript:extend-timeout-heavy-init
// Title: Extending timeout during heavy initialization
// Context: Plugins with large sample libraries or complex Builder
// API setups need to extend the compilation timeout to prevent
// HISE from aborting the initialization. Call this before or
// during the long-running operation.

// Extend by 10 seconds before a heavy initialization loop
Engine.extendTimeOut(10000);

// ... heavy initialization code (Builder API, sample loading, etc.)

// Can be called multiple times if needed
Engine.extendTimeOut(5000);
```
```json:testMetadata:extend-timeout-heavy-init
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Engine.getUptime() >= 0", "value": true}
  ]
}
```
