Registers a custom image provider that resolves `![alt](url)` markdown image links to actual images. Pass a JSON array of entries, each containing a `URL` and either path data or a pool image reference. The provider supports two entry types:

**Path entries** render a monochromatic vector icon from a Path object:

| Property | Type | Description |
|----------|------|-------------|
| `URL` | String | The URL to match in markdown image syntax |
| `Type` | `"Path"` | Must be `"Path"` |
| `Data` | Path or String | A Path object, or a base64 string encoding path data |
| `Colour` | Integer | Fill colour as ARGB integer (default: `0xFF888888`) |

**Image entries** load a bitmap from the HISE image pool:

| Property | Type | Description |
|----------|------|-------------|
| `URL` | String | The URL to match in markdown image syntax |
| `Type` | `"Image"` | Must be `"Image"` |
| `Reference` | String | Pool reference string (e.g. `"{PROJECT_FOLDER}image.png"`) |

You can control image size using non-standard syntax in the markdown link. Append a colon and a size value after the URL:

- `![](icon:80px)` - absolute size in pixels
- `![](icon:50%)` - relative size as a percentage of the available width

> [!Warning:Replaces all existing image resolvers] Calling this method replaces all existing image resolvers on the renderer, not just the scripted provider. Only the entries in the new array will resolve after this call.
