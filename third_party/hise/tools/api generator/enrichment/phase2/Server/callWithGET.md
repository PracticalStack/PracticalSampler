## callWithGET

**Examples:**

```javascript:version-check
// Title: Check for plugin updates via a JSON manifest
// Context: A common pattern for notifying users of available updates.
// The plugin fetches a small JSON file from a static URL and compares
// the version fields against the running version.

Server.setBaseURL("https://updates.example.com");

const var MAJOR_VERSION = 1;
const var MINOR_VERSION = 2;
const var PATCH_VERSION = 0;

inline function checkForUpdates()
{
    // Pass an empty string (not {}) when there are no query parameters
    Server.callWithGET("version.json", "", function(status, response)
    {
        if (status == Server.StatusOK)
        {
            // response is auto-parsed from JSON:
            // { "MajorVersion": 1, "MinorVersion": 3, "PatchVersion": 0, "DownloadLink": "..." }
            local isNewer = response.MajorVersion > MAJOR_VERSION ||
                            response.MinorVersion > MINOR_VERSION ||
                            response.PatchVersion > PATCH_VERSION;

            if (isNewer)
            {
                Console.print("Update available: " +
                    response.MajorVersion + "." +
                    response.MinorVersion + "." +
                    response.PatchVersion);
            }
        }
    });
};

checkForUpdates();
```
```json:testMetadata:version-check
{
  "testable": false,
  "skipReason": "requires-live-server"
}
```
