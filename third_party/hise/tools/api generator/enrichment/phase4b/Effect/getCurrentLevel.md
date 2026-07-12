Effect::getCurrentLevel(Integer leftChannel) -> Double

Thread safety: SAFE
Returns the current output level of the effect for the specified channel.
Pass true for left, false for right. Reads from the processor's DisplayValues
struct, updated during audio processing. Returns OUTPUT level (outL/outR),
not input level.
Required setup:
  const var fx = Synth.getEffect("MyEffect");
Anti-patterns:
  - Do NOT poll without smoothing -- raw level values fluctuate rapidly.
    Apply exponential decay (e.g., factor 0.77) in a ~30ms timer callback.
Source:
  ScriptingApiObjects.cpp:3373  ScriptingEffect::getCurrentLevel()
    -> leftChannel ? effect->getDisplayValues().outL : effect->getDisplayValues().outR
