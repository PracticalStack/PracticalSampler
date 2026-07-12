ScriptAudioWaveform::setPlaybackPosition(Double normalisedPosition) -> undefined

Thread safety: SAFE
Moves the playback cursor on the waveform to the given normalized 0..1 position.
Display-only -- does not affect actual audio playback.

Dispatch/mechanics:
  normalisedPosition * currentRange.getLength() -> sampleIndex
    -> ComplexDataUIUpdater::sendDisplayChangeMessage(sampleIndex, async)

Anti-patterns:
  - Do NOT assume this controls audio playback -- it only drives the visual cursor
  - Silently does nothing if no audio data is loaded

Pair with:
  getRangeStart/getRangeEnd -- define the sample range that the position maps into

Source:
  ScriptingApiContent.cpp  ScriptAudioWaveform::setPlaybackPosition()
    -> roundToInt(rangeLength * normalisedPosition)
    -> af->getUpdater().sendDisplayChangeMessage(sampleIndex, sendNotificationAsync, true)
