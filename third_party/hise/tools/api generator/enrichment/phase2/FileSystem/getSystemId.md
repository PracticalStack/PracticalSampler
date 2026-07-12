## getSystemId

**Examples:**

```javascript:installation-identifier
// Title: Generate a unique installation identifier for crash reporting
// Context: getSystemId() returns a deterministic hex string derived from
// hardware characteristics. It identifies a specific machine without
// exposing personal information, making it suitable for anonymous
// crash reports or usage analytics.

reg machineId = FileSystem.getSystemId();

Console.print("Machine ID: " + machineId);
Console.print("ID length: " + machineId.length);

// Include the machine ID in a crash report payload
inline function buildCrashReport(errorMessage)
{
    return {
        "machine_id": FileSystem.getSystemId(),
        "product": Engine.getProjectInfo().ProjectName,
        "version": Engine.getProjectInfo().ProjectVersion,
        "os": Engine.getOS(),
        "message": errorMessage
    };
};

// Verify determinism: calling it twice returns the same value
Console.print("Stable: " + (FileSystem.getSystemId() == machineId));
```
```json:testMetadata:installation-identifier
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "machineId.length > 0", "value": true},
    {"type": "REPL", "expression": "FileSystem.getSystemId() == machineId", "value": true}
  ]
}
```
