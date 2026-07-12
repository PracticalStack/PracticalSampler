WavetableController::setPostFXProcessors(Array postFXData) -> undefined

Thread safety: UNSAFE -- acquires CriticalSection (postFXLock), triggers killVoicesAndCall for safe re-rendering on the sample loading thread
Sets the post-processing effects chain applied to the wavetable after resynthesis.
Input is an array of JSON objects, each defining a processor type and optional
parameter range. After setting processors, wavetable is automatically re-rendered.

Dispatch/mechanics:
  Swaps new processor list under postFXLock -> renderPostFX(true) via killVoicesAndCall
    -> restores original data from lastResynthesisedData
    -> postProcessCycles() applies all PostFX to each cycle
    -> rebuildMipMaps() rebuilds wavetable from processed cycles

Pair with:
  resynthesise -- must have wavetable data loaded before applying post-FX

Anti-patterns:
  - Do NOT omit the "Type" property in processor objects -- required for each element
  - Do NOT use "Custom" type without setting TableProcessor and TableIndex --
    requires a connected Table for waveshaping

Source:
  ScriptingApiObjects.cpp  setPostFXProcessors()
    -> wt->setPostProcessors(Array<PostFXProcessor>&&, sendNotification)
    -> renderPostFX(true) -> killVoicesAndCall -> postProcessCycles() -> rebuildMipMaps()
  WavetableSynth.h:263  PostFXProcessor types: Identity, Custom, Sin, Warp,
    FM1-FM4, Sync, Root, Clip, Tanh, Bitcrush, SampleAndHold, Fold, Normalise, Phase
