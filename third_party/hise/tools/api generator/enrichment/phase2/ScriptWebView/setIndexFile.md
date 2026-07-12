## setIndexFile

**Examples:**

```javascript:set-index-file-full-setup
// Title: Full webview setup with file-based content
// Context: The standard initialization pattern: resolve the web content
// folder from the project structure, clear stale state, load the index file,
// and configure caching for development vs. export

const var wv = Content.addWebView("WebView1", 0, 0);
wv.set("width", 600);
wv.set("height", 400);

// Resolve the web content folder relative to the project.
// Placing it under Images/ ensures HISE embeds it on export
const var webRoot = FileSystem.getFolder(FileSystem.AudioFiles)
                              .getParentDirectory()
                              .getChildFile("Images/webview");

// Clear cached state from previous compilations during development
wv.reset();

// The index file's parent directory becomes the root for resolving
// relative paths to CSS, JS, and image resources
wv.setIndexFile(webRoot.getChildFile("index.html"));

// false during development for live reload, true for export
wv.set("enableCache", true);

// Replay callFunction/evaluate calls when a new webview appears
wv.set("enablePersistence", true);
```
```json:testMetadata:set-index-file-full-setup
{
  "testable": false,
  "skipReason": "Requires HTML files on disk at the expected path"
}
```

**Pitfalls:**
- The file path must be resolved via the `FileSystem` API to get a `File` object. String paths like `"/path/to/index.html"` cause a script error.
- On Windows, backslashes in file paths can cause issues when passed between HiseScript and JavaScript. Use `.replace("\\", "/")` when sending paths to the JS side.
