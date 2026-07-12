## startPerfettoTracing

**Examples:**

```javascript:perfetto-toggle-panel
// Title: Toggle Perfetto tracing via a hidden UI element
// Context: A ScriptPanel (e.g., the plugin logo) toggles profiling
// on click. First click starts recording, second click stops and
// opens the trace file.

const var ProfilePanel = Content.getComponent("LogoPanel");

ProfilePanel.setMouseCallback(function(event)
{
    if(event.clicked)
    {
        this.data.tracing = !this.data.tracing;

        if(this.data.tracing)
        {
            Settings.startPerfettoTracing();
        }
        else
        {
            var f = FileSystem.getFolder(FileSystem.Desktop).getChildFile("trace.pftrace");
            f.deleteFileOrDirectory();
            Settings.stopPerfettoTracing(f);
            f.show(); // open in file manager
        }

        this.repaint();
    }
    });
```

```json:testMetadata:perfetto-toggle-panel
{
  "testable": false,
  "skipReason": "Requires PERFETTO=1 compile flag; throws script error without it"
}
```
