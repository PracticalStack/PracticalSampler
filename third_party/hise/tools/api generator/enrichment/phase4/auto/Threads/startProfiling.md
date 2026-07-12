Starts a thread profiling session and calls the finish callback with Base64-encoded profiling data when recording completes. The first argument can be either a plain number (duration in milliseconds, 10-10000) or a JSON options object for fine-grained control:

| Property | Type | Description |
|----------|------|-------------|
| `recordingLength` | String | Duration string (e.g. `"1000 ms"`) |
| `recordingTrigger` | Number | 0 = Manual, 1 = Compilation, 2 = MIDI input, 3 = Mouse click |
| `threadFilter` | Array | Thread name strings to include (e.g. `"Audio Thread"`, `"UI Thread"`) |
| `eventFilter` | Array | Event type strings to include (e.g. `"Script"`, `"DSP"`, `"Lock"`) |

The easiest way to build the options object is to configure the profiling popup to your needs and click **Export as JSON** to generate a matching JSON object you can paste into your script. When using a JSON options object with a non-manual `recordingTrigger`, recording does not start immediately but waits for the specified trigger event.

Load the recorded data into the PerfettoWebViewer floating tile to visually inspect thread activity, lock contention, and callback timing.

> [!Warning:Enable profiling toolkit in project settings] In compiled plugins, the profiling toolkit must be explicitly enabled by adding `HISE_INCLUDE_PROFILING_TOOLKIT=1` to the ExtraDefinitions field in your project settings. Without it, this method throws a script error.
