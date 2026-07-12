AudioFile (object)
Obtain via: Engine.createAndRegisterAudioFile(index) or AudioSampleProcessor.getAudioFile(slotIndex)

Scriptable handle to a processor's audio file slot. Provides methods to load
audio files from pool references or programmatic buffers, control sample ranges
and loop points, query properties (length, sample rate), and receive callbacks
on content or playback position changes. Part of the complex data reference
system shared with Table, SliderPackData, and DisplayBuffer.

Complexity tiers:
  1. Basic file loading: AudioSampleProcessor.getAudioFile, loadFile. Load
     pool-referenced audio into processor slots (wavetables, IRs, one-shots).
  2. Content change monitoring: + Broadcaster.attachToComplexData("AudioFile.Content",
     moduleId, slotIndex). React to file loads and content changes across one or
     more processors. Preferred over setContentCallback for multi-slot monitoring.
  3. Programmatic buffer injection: + loadBuffer, getContent, update. Inject
     script-generated audio into processor slots or read-modify-write existing data.

Practical defaults:
  - Use Broadcaster.attachToComplexData("AudioFile.Content", moduleId, 0, "desc")
    rather than setContentCallback() when monitoring changes. Scales to multiple
    slots and integrates with the event bus architecture.
  - Use Engine.loadAudioFilesIntoPool() at init to get pool reference strings,
    then pass them to loadFile(). Standard pattern for embedded audio selection.
  - Obtain handles via Synth.getAudioSampleProcessor(id).getAudioFile(slotIndex)
    rather than Engine.createAndRegisterAudioFile(index). The former binds to a
    processor's audio slot. The standalone method is for script-owned data slots
    not tied to a specific processor.

Complex data chain:

![Audio File Data Chain](topology_complex-audio-data-chain.svg)

  - AudioSampleProcessor selects the module that owns one or more audio file slots.
  - AudioFile is the complex-data handle for one slot within that module.
  - ScriptAudioWaveform displays or edits one selected slot in the UI.

  Use the binding properties separately:
  - processorId selects the owning processor.
  - sampleIndex selects which audio slot inside that processor should be displayed.

  This is not the normal parameter binding path. parameterId targets processor
  parameters, while audio-slot binding uses sampleIndex instead.

Common mistakes:
  - Using getNumSamples() after setRange() expecting total file length --
    returns sub-range size. Use getTotalLengthInSamples() for original length.
  - Passing a filesystem path to loadFile() ("C:/audio/file.wav") -- expects a
    HISE pool reference ("{PROJECT_FOLDER}file.wav") or File.toString(0) output.
  - Linking to a different data type (af.linkTo(table)) -- throws "Type mismatch"
    error. linkTo requires both objects to be the same data type.
  - Registering setContentCallback() on each of 12+ AudioFile handles individually
    -- use one Broadcaster.attachToComplexData to monitor all slots at once.

Example:
  // Get an AudioFile handle from an AudioLoopPlayer
  const var asp = Synth.getAudioSampleProcessor("AudioLoopPlayer1");
  const var af = asp.getAudioFile(0);

  // Load a file and set up a content callback
  af.loadFile("{PROJECT_FOLDER}audio/loop.wav");

  af.setContentCallback(function(notification)
  {
      Console.print("Audio content changed, samples: " + af.getNumSamples());
  });

Methods (15):
  getContent                  getCurrentlyDisplayedIndex
  getCurrentlyLoadedFile      getLoopRange
  getNumSamples               getRange
  getSampleRate               getTotalLengthInSamples
  linkTo                      loadBuffer
  loadFile                    setContentCallback
  setDisplayCallback          setRange
  update
