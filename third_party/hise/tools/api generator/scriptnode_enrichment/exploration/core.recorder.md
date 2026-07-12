# core.recorder - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CoreNodes.h:266`
**Base class:** `data::base`
**Classification:** audio_processor

## Signal Path

Records the input audio into an internal buffer and flushes to the AudioFile slot when complete. The audio signal passes through completely unmodified -- the recorder only reads input samples to copy them into the recording buffer.

Recording flow:
1. `setState(1)` transitions to `RecordingState::Recording` and resets `recordingIndex` to 0
2. During processing, `processFrameInternal()` (line 397) writes each sample to `recordingBuffer` per channel
3. When `recordingIndex >= numSamplesInBuffer`, transitions to `WaitingForStop` and sets `updater->flushFlag`
4. The `InternalUpdater` (UI timer callback) calls `flush()` which loads the recording buffer into the `MultiChannelAudioBuffer` via `af->loadBuffer()`

The recording uses frame-based processing internally. `process()` dispatches to `processFix<PD, C>()` which converts to frame iteration.

## Gap Answers

### recording-mechanism: How does recording work?

The recorder reads input samples per-frame and writes them to an internal `AudioSampleBuffer recordingBuffer`. When recording is complete (buffer full), it asynchronously flushes to the AudioFile slot via a UI timer callback. Audio passes through unmodified during recording.

### recording-length-zero: What does RecordingLength=0 mean?

RecordingLength=0 means `bufferSize = 0 * sampleRate = 0` samples. The `rebuildBuffer()` method (line 380) creates a buffer of this size. In `processFrameInternal()`, `isPositiveAndBelow(0, 0)` is false, so no recording occurs. Effectively, RecordingLength=0 means "do not record". The user must set a positive value.

### state-values: Is State simply On/Off?

Yes, binary. `setState()` (line 332): `state > 0.5` = Recording, else Idle. The parameter has `setParameterValueNames({"On", "Off"})`. There is a third internal state `WaitingForStop` but it is not user-accessible -- it occurs automatically when the buffer is full.

### audio-file-interaction: How does recording interact with AudioFile?

The recording buffer is pre-allocated via `rebuildBuffer()` based on RecordingLength and sampleRate. When recording completes, `flush()` calls `af->loadBuffer(recordingBuffer, sampleRate)` which loads the audio data into the `MultiChannelAudioBuffer`. The AudioFile is then available for playback via core.file_player. `setExternalData()` also disables SampleMap and SFZ providers on the AudioFile slot.

### channel-handling: Mono or stereo recording?

Records all input channels up to the buffer's channel count. In `processFrameInternal()` (line 403): `for (int i = 0; i < data.size(); i++) recordingBuffer.setSample(i, recordingIndex, data[i])`. The channel count of `recordingBuffer` is set from `lastSpecs.numChannels` in `rebuildBuffer()`. Supports 1-2 channels (process dispatches for 1 or 2 channels).

## Parameters

- **State** (On/Off, default Off): Start/stop recording. Transitioning to Recording resets the recording position.
- **RecordingLength** (0-2000 ms, default 0): Duration to record. Buffer is allocated from this.

## CPU Assessment

baseline: negligible (when idle)
polyphonic: false
scalingFactors: [{"parameter": "State", "impact": "low", "note": "Recording adds per-sample buffer write overhead"}]
