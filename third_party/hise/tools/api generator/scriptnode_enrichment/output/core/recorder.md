---
title: Recorder
description: "Records the input signal into an audio file slot for playback or UI display."
factoryPath: core.recorder
factory: core
polyphonic: false
tags: [core, recorder, audio-file, capture]
screenshot: /images/v2/reference/scriptnodes/core/recorder.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors:
    - { parameter: "State", impact: low, note: "Recording adds per-sample buffer write overhead" }
forumReferences:
  - { tid: 7947, reason: "Recorder workflow: external slot, length, trigger" }
seeAlso:
  - { id: "core.file_player", type: companion, reason: "Plays back audio from the same audio file slot" }
commonMistakes:
  - title: "RecordingLength must be set before recording"
    wrong: "Leaving RecordingLength at 0 and toggling State to On"
    right: "Set RecordingLength to the desired duration in milliseconds before starting recording."
    explanation: "A RecordingLength of 0 allocates no buffer, so no samples are captured. Always set a positive value first."
  - title: "Audio file slot must be set to External for shared playback"
    wrong: "Recording audio and expecting a file_player or granulator to pick it up without configuring the slot"
    right: "Set the recorder's audio file slot to External and assign the same external slot to the playback node."
    explanation: "The recorder writes to an internal audio file slot. For another node to read the recorded audio, both nodes must share the same external slot reference."
llmRef: |
  core.recorder

  Records the input audio into an internal buffer and writes it to an audio file slot when recording completes. Audio passes through unmodified during recording.

  Signal flow:
    audio in -> record to buffer (when State=On) -> flush to AudioFile slot
    audio in -> audio out (passthrough)

  CPU: negligible (idle), low (recording)

  Parameters:
    State (Off / On, default Off): starts or stops recording
    RecordingLength (0 - 2000 ms, default 0): recording duration in milliseconds

  When to use:
    Rarely used (rank 108, 1 instance). Use to capture audio snippets for playback via core.file_player, for live sampling, or for UI display via the DisplayBufferSource API.

  Common mistakes:
    RecordingLength must be set before recording -- a value of 0 captures nothing.
    Audio file slot must be set to External for shared playback with other nodes.

  Forum references: tid:7947 (recorder workflow and shared external slot)

  See also:
    [companion] core.file_player -- plays back from the same audio file slot
---

![Node screenshot](/images/custom/scriptnode/recorder.png)

This node records the input audio signal into an internal buffer and writes it to an audio file slot when recording completes. The audio signal passes through completely unmodified during recording. When the buffer is full, the recorded audio is automatically flushed to the audio file slot, where it can be picked up by other nodes (such as [core.file_player]($SN.core.file_player$)) or displayed on the UI using the [DisplayBufferSource]($API.DisplayBufferSource$) scripting API.

The recorder supports mono and stereo input, matching the channel count of its container. Recording starts when State is set to On and stops automatically when the buffer reaches the configured RecordingLength. Setting State to On again resets the recording position and starts a new capture. The audio file slot is written asynchronously when the buffer is full, after which the recorded data is available to any node referencing the same audio file slot.

## Signal Path

::signal-path
---
glossary:
  parameters:
    State:
      desc: "Starts or stops recording"
      range: "Off / On"
      default: "Off"
    RecordingLength:
      desc: "Duration of the recording buffer in milliseconds"
      range: "0 - 2000 ms"
      default: "0"
  functions:
    recordToBuffer:
      desc: "Copies input samples into the internal recording buffer"
    flushToFile:
      desc: "Writes the completed recording to the audio file slot"
---

```
// core.recorder - record input to audio file slot
// audio in -> audio out (passthrough)

process(input) {
    if (State == On) {
        recordToBuffer(input)
        if (bufferFull)
            flushToFile()
    }
    // audio passes through unchanged
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Recording
    params:
      - { name: State, desc: "Starts or stops recording. Setting to On resets the recording position.", range: "Off / On", default: "Off" }
      - { name: RecordingLength, desc: "Duration of the recording buffer. Must be set to a positive value before recording.", range: "0 - 2000 ms", default: "0" }
---
::

### Recording workflow

The complete recording workflow is:

1. Assign the recorder an **external** audio file slot, shared with a playback node (such as [core.file_player]($SN.core.file_player$) or [core.granulator]($SN.core.granulator$)).
2. Set **RecordingLength** to the desired capture duration in milliseconds.
3. Set **State** to On to begin recording. The recorder captures the predetermined length and then stops automatically.

When recording completes, the buffer in the shared external slot is immediately available to the connected playback node. Setting State to On again resets the position and starts a new capture.

**See also:** $SN.core.file_player$ -- plays back audio from the same audio file slot
