# Settings -- Methods

## clearMidiLearn

**Signature:** `undefined clearMidiLearn()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls `MidiControlAutomationHandler::clear()` with `sendNotification`, which triggers listener updates and potential UI repaints.
**Minimal Example:** `Settings.clearMidiLearn();`

**Description:**
Removes all MIDI controller-to-parameter mappings from the MIDI automation handler. Every MIDI learn assignment is cleared and a notification is sent to update the UI.

**Parameters:**
None.

---

## crashAndBurn

**Signature:** `undefined crashAndBurn()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `Settings.crashAndBurn();`

**Description:**
Deliberately crashes the process by dereferencing a null pointer, used for testing crash reporting and stack traces. In backend builds, if `CompileWithDebugSymbols` is not enabled in project settings, a script error is thrown first -- in safe-check mode this prevents the crash, giving the user a chance to enable debug symbols for a meaningful stack trace.

**Parameters:**
None.

**Pitfalls:**
- In frontend/exported builds, crashes unconditionally with no prerequisite check. Only use this for testing crash reporting in controlled environments.

**Cross References:**
- `$API.Settings.setEnableDebugMode$`

---

## getAvailableBufferSizes

**Signature:** `Array getAvailableBufferSizes()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Queries the audio device manager and constructs an Array with heap allocations.
**Minimal Example:** `var sizes = Settings.getAvailableBufferSizes();`

**Description:**
Returns an array of available buffer sizes (as integers) for the currently selected audio device. Returns an empty array if no audio device is active.

**Parameters:**
None.

**Pitfalls:**
- Returns an empty array (not an error) when no audio device is selected. Check the array length before accessing elements.

**Cross References:**
- `$API.Settings.getCurrentBufferSize$`
- `$API.Settings.setBufferSize$`

---

## getAvailableDeviceNames

**Signature:** `Array getAvailableDeviceNames()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Queries audio device type for device names, constructs String array on heap.
**Minimal Example:** `var names = Settings.getAvailableDeviceNames();`

**Description:**
Returns an array of audio device names (as strings) available for the currently selected audio device type. Returns an empty array if no device type is configured.

**Parameters:**
None.

**Pitfalls:**
- The returned device names correspond to the current device type. Change the device type with `setAudioDeviceType()` first to get names for a different driver.

**Cross References:**
- `$API.Settings.getCurrentAudioDevice$`
- `$API.Settings.setAudioDevice$`
- `$API.Settings.getAvailableDeviceTypes$`

---

## getAvailableDeviceTypes

**Signature:** `Array getAvailableDeviceTypes()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Iterates device type array and constructs String array on heap.
**Minimal Example:** `var types = Settings.getAvailableDeviceTypes();`

**Description:**
Returns an array of available audio device type names (as strings), such as "Windows Audio", "ASIO", "DirectSound", or "CoreAudio" depending on platform.

**Parameters:**
None.

**Cross References:**
- `$API.Settings.getCurrentAudioDeviceType$`
- `$API.Settings.setAudioDeviceType$`

---

## getAvailableOutputChannels

**Signature:** `Array getAvailableOutputChannels()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Queries audio device and constructs String array on heap.
**Minimal Example:** `var channels = Settings.getAvailableOutputChannels();`

**Description:**
Returns an array of output channel pair names (as strings) for the currently selected audio device. Channel pairs are stereo pairs, not individual channels. Returns an empty array if no audio device is active.

**Parameters:**
None.

**Cross References:**
- `$API.Settings.getCurrentOutputChannel$`
- `$API.Settings.setOutputChannel$`

---

## getAvailableSampleRates

**Signature:** `Array getAvailableSampleRates()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Queries audio device and constructs String array on heap.
**Minimal Example:** `var rates = Settings.getAvailableSampleRates();`

**Description:**
Returns an array of supported sample rates for the currently selected audio device. Returns an empty array if no audio device is active.

**Parameters:**
None.

**Pitfalls:**
- [BUG] Returns an array of strings (e.g., `"44100"`, `"48000"`), not numbers. This differs from `getAvailableBufferSizes()` which returns integers. Parse values with `parseInt()` if arithmetic is needed.

**Cross References:**
- `$API.Settings.getCurrentSampleRate$`
- `$API.Settings.setSampleRate$`

---

## getCurrentAudioDevice

**Signature:** `String getCurrentAudioDevice()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Queries audio device manager and constructs a String.
**Minimal Example:** `var device = Settings.getCurrentAudioDevice();`

**Description:**
Returns the name of the currently active audio device. Returns an empty string if no audio device is selected.

**Parameters:**
None.

**Cross References:**
- `$API.Settings.getAvailableDeviceNames$`
- `$API.Settings.setAudioDevice$`

---

## getCurrentAudioDeviceType

**Signature:** `String getCurrentAudioDeviceType()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Queries audio device manager and constructs a String.
**Minimal Example:** `var type = Settings.getCurrentAudioDeviceType();`

**Description:**
Returns the type name of the currently active audio device (e.g., "ASIO", "Windows Audio", "CoreAudio"). Returns an empty string if no audio device is selected.

**Parameters:**
None.

**Cross References:**
- `$API.Settings.getAvailableDeviceTypes$`
- `$API.Settings.setAudioDeviceType$`

---

## getCurrentBufferSize

**Signature:** `Integer getCurrentBufferSize()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Accesses device manager to query current block size.
**Minimal Example:** `var size = Settings.getCurrentBufferSize();`

**Description:**
Returns the current audio buffer size in samples. Returns 0 if no audio device manager is available.

**Parameters:**
None.

**Pitfalls:**
- Returns 0 (not -1) when no device is available, unlike `getCurrentSampleRate()` which returns -1. Check whether a device is selected before interpreting the value.

**Cross References:**
- `$API.Settings.getAvailableBufferSizes$`
- `$API.Settings.setBufferSize$`

---

## getCurrentOutputChannel

**Signature:** `Integer getCurrentOutputChannel()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Queries audio device for active output channel bits.
**Minimal Example:** `var ch = Settings.getCurrentOutputChannel();`

**Description:**
Returns the index of the currently active output channel pair. The index represents a stereo pair, not an individual channel. Returns 0 if no audio device is active.

**Parameters:**
None.

**Cross References:**
- `$API.Settings.getAvailableOutputChannels$`
- `$API.Settings.setOutputChannel$`

---

## getCurrentSampleRate

**Signature:** `Double getCurrentSampleRate()`
**Return Type:** `Double`
**Call Scope:** unsafe
**Call Scope Note:** Queries audio device for current sample rate.
**Minimal Example:** `var sr = Settings.getCurrentSampleRate();`

**Description:**
Returns the current audio sample rate in Hz. Returns -1 if no audio device is active.

**Parameters:**
None.

**Cross References:**
- `$API.Settings.getAvailableSampleRates$`
- `$API.Settings.setSampleRate$`

---

## getCurrentVoiceMultiplier

**Signature:** `Integer getCurrentVoiceMultiplier()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var mult = Settings.getCurrentVoiceMultiplier();`

**Description:**
Returns the current voice amount multiplier. The default value is 2. This multiplier is applied to the base voice count to determine the total number of available voices.

**Parameters:**
None.

**Cross References:**
- `$API.Settings.setVoiceMultiplier$`

---

## getDiskMode

**Signature:** `Integer getDiskMode()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var mode = Settings.getDiskMode();`

**Description:**
Returns the current disk streaming mode. 0 = SSD (larger preload buffer, optimized for fast storage), 1 = HDD (stream-optimized for slower storage).

**Parameters:**
None.

**Cross References:**
- `$API.Settings.setDiskMode$`

---

## getMidiInputDevices

**Signature:** `Array getMidiInputDevices()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Calls `MidiInput::getDevices()` which queries system MIDI devices and constructs a StringArray.
**Minimal Example:** `var devices = Settings.getMidiInputDevices();`

**Description:**
Returns an array of available MIDI input device names (as strings) detected by the system.

**Parameters:**
None.

**Cross References:**
- `$API.Settings.toggleMidiInput$`
- `$API.Settings.isMidiInputEnabled$`

---

## getUserDesktopSize

**Signature:** `Array getUserDesktopSize()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Queries the Desktop singleton for display information, constructs an Array.
**Minimal Example:** `var size = Settings.getUserDesktopSize();`

**Description:**
Returns a two-element array `[width, height]` representing the main display's usable area in pixels. The usable area excludes the taskbar and other system-reserved regions.

**Parameters:**
None.

**Example:**
```javascript:desktop-size-zoom
// Title: Query usable desktop dimensions
var desktop = Settings.getUserDesktopSize();
var maxWidth = desktop[0];
var maxHeight = desktop[1];
Console.print("Desktop: " + maxWidth + " x " + maxHeight);
```

```json:testMetadata:desktop-size-zoom
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "desktop.length", "value": 2},
    {"type": "REPL", "expression": "desktop[0] > 0", "value": true},
    {"type": "REPL", "expression": "desktop[1] > 0", "value": true}
  ]
}
```

**Cross References:**
- `$API.Settings.getZoomLevel$`
- `$API.Settings.setZoomLevel$`

---

## getZoomLevel

**Signature:** `Double getZoomLevel()`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var zoom = Settings.getZoomLevel();`

**Description:**
Returns the current global UI scale factor. A value of 1.0 means 100% zoom (no scaling).

**Parameters:**
None.

**Cross References:**
- `$API.Settings.setZoomLevel$`

---

## isIppEnabled

**Signature:** `Integer isIppEnabled(Integer returnTrueIfMacOS)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var hasIpp = Settings.isIppEnabled(true);`

**Description:**
Checks whether Intel Performance Primitives (IPP) is available for accelerated FFT. On Windows, returns a compile-time constant reflecting whether HISE was built with `USE_IPP=1`. On macOS and other platforms, returns the value of `returnTrueIfMacOS` -- this parameter exists because macOS has vDSP (Apple's equivalent fast FFT), so passing `true` means "fast FFT is available on this platform."

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| returnTrueIfMacOS | Integer | no | Value to return on non-Windows platforms. Pass `true` if macOS's vDSP is an acceptable substitute for IPP. | Boolean |

**Example:**
```javascript:check-fft-acceleration
// Title: Check for fast FFT support across platforms
var hasFastFFT = Settings.isIppEnabled(true);
Console.print(hasFastFFT ? "Fast FFT available" : "No hardware FFT acceleration");
```

```json:testMetadata:check-fft-acceleration
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "hasFastFFT == 0 || hasFastFFT == 1", "value": true}
}
```

---

## isMidiChannelEnabled

**Signature:** `Integer isMidiChannelEnabled(Integer index)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Accesses the main synth chain's channel filter data.
**Minimal Example:** `var enabled = Settings.isMidiChannelEnabled(1);`

**Description:**
Returns whether a specific MIDI channel is enabled. Index 0 checks if all channels are enabled. Indices 1-16 check individual MIDI channels (1-based, matching standard MIDI channel numbering).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | MIDI channel index. 0 = all channels query, 1-16 = individual channel. | 0-16 |

**Cross References:**
- `$API.Settings.toggleMidiChannel$`

---

## isMidiInputEnabled

**Signature:** `Integer isMidiInputEnabled(String midiInputName)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Queries the device manager for MIDI input state.
**Minimal Example:** `var enabled = Settings.isMidiInputEnabled("My MIDI Device");`

**Description:**
Returns whether the specified MIDI input device is currently enabled. Returns `false` if no device manager is available.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| midiInputName | String | no | The exact name of the MIDI input device as returned by `getMidiInputDevices()`. | Must match an available device name |

**Cross References:**
- `$API.Settings.getMidiInputDevices$`
- `$API.Settings.toggleMidiInput$`

---

## isOpenGLEnabled

**Signature:** `Integer isOpenGLEnabled()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var gl = Settings.isOpenGLEnabled();`

**Description:**
Returns whether OpenGL rendering is enabled. This reads the stored flag -- it may not reflect the actual rendering state if `setEnableOpenGL()` was called recently, since the OpenGL context change is deferred until the next interface rebuild.

**Parameters:**
None.

**Cross References:**
- `$API.Settings.setEnableOpenGL$`

---

## setAudioDevice

**Signature:** `undefined setAudioDevice(String name)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to AudioProcessorDriver which reconfigures the audio device, involving I/O and potential lock acquisition.
**Minimal Example:** `Settings.setAudioDevice("Built-in Output");`

**Description:**
Sets the active audio output device by name. The name must match one of the strings returned by `getAvailableDeviceNames()`. Primarily useful in standalone builds where the application manages its own audio device.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| name | String | no | The audio device name to activate. | Must match an available device name |

**Cross References:**
- `$API.Settings.getAvailableDeviceNames$`
- `$API.Settings.getCurrentAudioDevice$`

---

## setAudioDeviceType

**Signature:** `undefined setAudioDeviceType(String deviceName)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Reconfigures the audio driver type, involving device manager state changes.
**Minimal Example:** `Settings.setAudioDeviceType("ASIO");`

**Description:**
Sets the audio device driver type by name (e.g., "ASIO", "Windows Audio", "CoreAudio"). The name must match one of the strings returned by `getAvailableDeviceTypes()`. Changing the device type resets the active audio device. Primarily useful in standalone builds.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| deviceName | String | no | The audio device type name to activate. | Must match an available device type |

**Cross References:**
- `$API.Settings.getAvailableDeviceTypes$`
- `$API.Settings.getCurrentAudioDeviceType$`

---

## setBufferSize

**Signature:** `undefined setBufferSize(Integer newBlockSize)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Reconfigures audio device buffer size through the device manager.
**Minimal Example:** `Settings.setBufferSize(512);`

**Description:**
Sets the audio buffer size in samples. The value should be one of the sizes returned by `getAvailableBufferSizes()`. Does nothing if no device manager is available. Primarily useful in standalone builds.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newBlockSize | Integer | no | Buffer size in samples. | Should match an available buffer size |

**Cross References:**
- `$API.Settings.getAvailableBufferSizes$`
- `$API.Settings.getCurrentBufferSize$`

---

## setDiskMode

**Signature:** `undefined setDiskMode(Integer mode)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Sets the disk mode on the sample manager, which may trigger streaming configuration changes.
**Minimal Example:** `Settings.setDiskMode(0);`

**Description:**
Sets the disk streaming mode. 0 = SSD (uses larger preload buffers, optimized for fast solid-state storage), 1 = HDD (stream-optimized for slower hard disk storage).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| mode | Integer | no | Disk mode index. | 0 (SSD) or 1 (HDD) |

**Pitfalls:**
- [BUG] No range validation is performed. Values other than 0 or 1 are accepted silently and cast directly to the internal DiskMode enum, producing undefined behavior.

**Cross References:**
- `$API.Settings.getDiskMode$`

---

## setEnableDebugMode

**Signature:** `undefined setEnableDebugMode(Integer shouldBeEnabled)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Starts or stops the debug logger, which involves file I/O.
**Minimal Example:** `Settings.setEnableDebugMode(true);`

**Description:**
Enables or disables the debug logger. When enabled, HISE logs detailed diagnostic information to a file for troubleshooting. When disabled, logging stops.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeEnabled | Integer | no | `true` to start logging, `false` to stop. | Boolean |

**Cross References:**
- `$API.Settings.crashAndBurn$`

---

## setEnableOpenGL

**Signature:** `undefined setEnableOpenGL(Integer shouldBeEnabled)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `Settings.setEnableOpenGL(true);`

**Description:**
Enables or disables OpenGL rendering. This sets an internal flag but does not immediately create or destroy the OpenGL context -- the change takes effect on the next interface rebuild.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeEnabled | Integer | no | `true` to enable OpenGL, `false` to disable. | Boolean |

**Pitfalls:**
- The change is deferred. `isOpenGLEnabled()` reflects the new flag value immediately, but actual rendering does not switch until the interface is rebuilt.

**Cross References:**
- `$API.Settings.isOpenGLEnabled$`

---

## setOutputChannel

**Signature:** `undefined setOutputChannel(Integer index)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls `CustomSettingsWindow::flipEnablement()` which modifies device manager channel configuration.
**Minimal Example:** `Settings.setOutputChannel(0);`

**Description:**
Selects an output channel pair by stereo pair index. The index corresponds to a stereo pair from `getAvailableOutputChannels()`, not an individual channel number. Primarily useful in standalone builds.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | Stereo pair index to activate. | Must be a valid pair index |

**Pitfalls:**
- The index is a stereo pair index (0 = first pair, 1 = second pair), not an individual channel number. Passing a channel number produces unexpected results.

**Cross References:**
- `$API.Settings.getAvailableOutputChannels$`
- `$API.Settings.getCurrentOutputChannel$`

---

## setSampleFolder

**Signature:** `undefined setSampleFolder(ScriptObject sampleFolder)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates link files or sets frontend sample location, involving file I/O.
**Minimal Example:** `Settings.setSampleFolder(FileSystem.getFolder(FileSystem.Samples));`

**Description:**
Sets the sample folder location. Accepts a `File` object (not a string path). In backend builds, creates a link file pointing to the new location. In frontend/exported builds, sets the sample location directly.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sampleFolder | ScriptObject | no | A `File` object pointing to the target sample directory. | Must be a File object pointing to an existing directory |

**Pitfalls:**
- [BUG] Silently does nothing if the argument is not a `File` object or if the path is not a directory. No error is reported.

**Cross References:**
- `$API.FileSystem.getFolder$`

---

## setSampleRate

**Signature:** `undefined setSampleRate(Double sampleRate)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Reconfigures audio device sample rate through the driver.
**Minimal Example:** `Settings.setSampleRate(48000.0);`

**Description:**
Sets the audio sample rate in Hz. The value should be one of the rates returned by `getAvailableSampleRates()`. Primarily useful in standalone builds.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sampleRate | Double | no | Sample rate in Hz. | Should match an available sample rate |

**Cross References:**
- `$API.Settings.getAvailableSampleRates$`
- `$API.Settings.getCurrentSampleRate$`

---

## setVoiceMultiplier

**Signature:** `undefined setVoiceMultiplier(Integer newVoiceAmount)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `Settings.setVoiceMultiplier(4);`

**Description:**
Sets the voice amount multiplier. This value is multiplied with the base voice count to determine total available voices. The default is 2.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newVoiceAmount | Integer | no | Voice multiplier value. | Positive integer |

**Pitfalls:**
- [BUG] No validation is performed. The value is stored directly without range checking. Negative or zero values are accepted silently.

**Cross References:**
- `$API.Settings.getCurrentVoiceMultiplier$`

---

## setZoomLevel

**Signature:** `undefined setZoomLevel(Double newLevel)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls `setGlobalScaleFactor()` with `sendNotificationAsync`, which triggers UI listener notifications.
**Minimal Example:** `Settings.setZoomLevel(1.5);`

**Description:**
Sets the global UI scale factor. The value is clamped to the range [0.25, 2.0] (25% to 200% zoom). A notification is sent asynchronously to update the UI.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newLevel | Double | no | Scale factor where 1.0 = 100%. | Clamped to 0.25-2.0 |

**Pitfalls:**
- Values outside [0.25, 2.0] are silently clamped, not rejected. Passing 3.0 results in 2.0 with no error.

**Cross References:**
- `$API.Settings.getZoomLevel$`

---

## startPerfettoTracing

**Signature:** `undefined startPerfettoTracing()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Begins a Perfetto profiling session, involving system-level tracing infrastructure.
**Minimal Example:** `Settings.startPerfettoTracing();`

**Description:**
Starts a Perfetto performance tracing session. Requires HISE to be compiled with `PERFETTO=1`. If Perfetto is not enabled, throws a script error.

**Parameters:**
None.

**Cross References:**
- `$API.Settings.stopPerfettoTracing$`

---

## stopPerfettoTracing

**Signature:** `undefined stopPerfettoTracing(ScriptObject traceFileToUse)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Ends the Perfetto session and writes trace data to disk.
**Minimal Example:** `Settings.stopPerfettoTracing(FileSystem.getFolder(FileSystem.Desktop).getChildFile("trace.pftrace"));`

**Description:**
Stops the active Perfetto tracing session and writes the trace data to the specified file. Requires HISE to be compiled with `PERFETTO=1`. The file must have a `.pftrace` extension.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| traceFileToUse | ScriptObject | no | A `File` object where the trace data will be written. | Must be a File with `.pftrace` extension |

**Pitfalls:**
- [BUG] The file extension is validated after the trace session has already ended and data written. If the extension is wrong, the trace data is saved to the misnamed file, then a script error is thrown. The data is not lost but Perfetto tools may not recognize the file without the correct extension.

**Cross References:**
- `$API.Settings.startPerfettoTracing$`

---

## toggleMidiChannel

**Signature:** `undefined toggleMidiChannel(Integer index, Integer value)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Accesses the main synth chain's channel filter data.
**Minimal Example:** `Settings.toggleMidiChannel(1, true);`

**Description:**
Enables or disables a MIDI channel for the main synth chain. Index 0 toggles all channels at once. Indices 1-16 target individual MIDI channels (1-based, matching standard MIDI channel numbering). Internally, 1-based indices are converted to 0-based for the channel filter.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | MIDI channel. 0 = all channels, 1-16 = individual channel. | 0-16 |
| value | Integer | no | `true` to enable, `false` to disable. | Boolean |

**Example:**
```javascript:midi-channel-filter
// Title: Enable only MIDI channel 1, disable all others
Settings.toggleMidiChannel(0, false);
Settings.toggleMidiChannel(1, true);
var ch1 = Settings.isMidiChannelEnabled(1);
Console.print("Channel 1 enabled: " + ch1);
```

```json:testMetadata:midi-channel-filter
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Settings.isMidiChannelEnabled(1)", "value": 1}
  ]
}
```

**Cross References:**
- `$API.Settings.isMidiChannelEnabled$`

---

## toggleMidiInput

**Signature:** `undefined toggleMidiInput(String midiInputName, Integer enableInput)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to AudioProcessorDriver which modifies MIDI device configuration.
**Minimal Example:** `Settings.toggleMidiInput("My MIDI Keyboard", true);`

**Description:**
Enables or disables a MIDI input device by name. The name must match one of the strings returned by `getMidiInputDevices()`. Primarily useful in standalone builds.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| midiInputName | String | no | The exact name of the MIDI input device. | Must match an available device name |
| enableInput | Integer | no | `true` to enable, `false` to disable. | Boolean |

**Cross References:**
- `$API.Settings.getMidiInputDevices$`
- `$API.Settings.isMidiInputEnabled$`
