Settings (namespace)

Audio device, MIDI input, zoom, disk mode, and OpenGL configuration namespace.
Delegates to GlobalSettingManager (zoom, disk mode, voice multiplier, OpenGL),
AudioProcessorDriver (audio device, buffer, sample rate, MIDI inputs), and
MainController (MIDI channels, debug logging, MIDI learn). Many audio device
methods are primarily useful in standalone builds.

Complexity tiers:
  1. Zoom control: getZoomLevel, setZoomLevel. Nearly every plugin implements
     this. Pair with Content.getScreenBounds for screen-aware clamping.
  2. Standalone configuration: + setAudioDevice, setAudioDeviceType,
     setBufferSize, setSampleRate, getMidiInputDevices, toggleMidiInput.
     Only needed for standalone builds where the app manages audio hardware.
  3. Advanced settings: + setSampleFolder for sample relocation,
     setEnableOpenGL for graphics quality tiers, startPerfettoTracing/
     stopPerfettoTracing for development profiling.

Practical defaults:
  - Use 0.25 zoom step increments for drag-to-zoom. Gives clean values
    (0.75, 1.0, 1.25, 1.5) that feel natural.
  - Clamp zoom to screen bounds using
    Content.getScreenBounds(false)[3] / interfaceHeight as the maximum,
    not just the Settings 2.0 ceiling.
  - After toggling OpenGL with setEnableOpenGL, show a message box telling
    the user to reload the plugin -- the change does not take effect until
    the next interface rebuild.
  - In standalone first-run, auto-enable all detected MIDI inputs so the
    user can play immediately.

Common mistakes:
  - Calling setZoomLevel without screen bounds check -- interface can grow
    larger than the display. Clamp to
    Content.getScreenBounds(false)[3] / interfaceHeight first.
  - Passing a string path to setSampleFolder -- requires a File object.
    String paths are silently ignored with no error.
  - Calling setEnableOpenGL and expecting immediate effect -- deferred
    until the next interface rebuild. Show a reload message.
  - Using toggleMidiChannel(1, true) expecting "all channels" -- index 0
    controls all channels; indices 1-16 are individual MIDI channels.
  - Calling Perfetto methods without PERFETTO=1 compile flag -- throws a
    script error at runtime.

Example:
  // Settings is a global namespace -- no instantiation needed.
  // Query current audio configuration
  var sampleRate = Settings.getCurrentSampleRate();
  var bufferSize = Settings.getCurrentBufferSize();
  var zoomLevel = Settings.getZoomLevel();

Methods (36):
  clearMidiLearn              crashAndBurn
  getAvailableBufferSizes     getAvailableDeviceNames
  getAvailableDeviceTypes     getAvailableOutputChannels
  getAvailableSampleRates     getCurrentAudioDevice
  getCurrentAudioDeviceType   getCurrentBufferSize
  getCurrentOutputChannel     getCurrentSampleRate
  getCurrentVoiceMultiplier   getDiskMode
  getMidiInputDevices         getUserDesktopSize
  getZoomLevel                isIppEnabled
  isMidiChannelEnabled        isMidiInputEnabled
  isOpenGLEnabled             setAudioDevice
  setAudioDeviceType          setBufferSize
  setDiskMode                 setEnableDebugMode
  setEnableOpenGL             setOutputChannel
  setSampleFolder             setSampleRate
  setVoiceMultiplier          setZoomLevel
  startPerfettoTracing        stopPerfettoTracing
  toggleMidiChannel           toggleMidiInput
