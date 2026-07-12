ChildSynth::asSampler() -> Sampler

Thread safety: SAFE
Attempts to cast this ChildSynth to a Sampler reference. Returns a Sampler handle
if the wrapped synth is a ModulatorSampler, undefined otherwise. Does not throw errors
on non-sampler types -- check the result with isDefined().
Dispatch/mechanics:
  dynamic_cast<ModulatorSampler*>(synth.get())
    -> if successful: returns new ScriptingApi::Sampler wrapping the cast pointer
    -> if fails: returns var() (undefined) silently -- intentional, no error
Pair with:
  Synth.getChildSynth -- obtain the ChildSynth reference first
Anti-patterns:
  - Do NOT call Sampler methods on the result without checking isDefined() first --
    undefined return is expected for non-sampler child synths
  - Do NOT rely on asSampler() to validate object existence -- if the underlying
    synth was deleted, it still creates a Sampler wrapping nullptr
Source:
  ScriptingApiObjects.cpp  asSampler()
    -> dynamic_cast<ModulatorSampler*>(synth.get())
    -> returns new Sampler(getScriptProcessor(), s) or var()
