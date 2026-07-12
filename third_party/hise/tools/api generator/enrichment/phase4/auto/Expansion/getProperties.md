Returns a JSON object containing this expansion's metadata. The standard properties are:

| Property | Default |
|----------|---------|
| `Name` | Folder name |
| `Version` | `"1.0.0"` |
| `Tags` | `""` |
| `ProjectName` | From project settings |
| `ProjectVersion` | From project settings |

Additional properties defined in the expansion's `expansion_info.xml` (such as `Description`, `Company`, or `UUID`) are also included. For encrypted expansions with the unlocker system enabled, a `Blowfish-Key` property is present - avoid displaying it on your UI.
