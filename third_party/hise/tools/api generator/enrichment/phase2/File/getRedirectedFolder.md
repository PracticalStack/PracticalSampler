## getRedirectedFolder

**Examples:**

```javascript:resolve-link-redirect
// Title: Resolve a folder that may have a HISE link redirect
// Context: Accessing the actual preset or sample directory, which may be
//          redirected to an external drive via a LinkWindows/LinkOSX file

const var presetDir = FileSystem.getFolder(FileSystem.UserPresets);

// Follow any link file redirect to the actual location
const var resolvedDir = presetDir.getChildFile("Presets").getRedirectedFolder();

Console.print("Actual preset location: " + resolvedDir.toString(0));

// Now use the resolved directory for file operations
var presetFiles = FileSystem.findFiles(resolvedDir, "*.preset", true);
Console.print("Found " + presetFiles.length + " presets");
```
```json:testMetadata:resolve-link-redirect
{
  "testable": false,
  "skipReason": "Output depends on host machine's filesystem layout and link file configuration"
}
```

**Pitfalls:**
- This method is only meaningful for directories. Calling it on a path that exists as a regular file triggers a script error. However, if the path does not exist at all, it silently returns the same File object.
- The link file name is platform-specific: `LinkWindows` on Windows, `LinkOSX` on macOS, `LinkLinux` on Linux. A redirect configured on one platform will not be followed on another.
