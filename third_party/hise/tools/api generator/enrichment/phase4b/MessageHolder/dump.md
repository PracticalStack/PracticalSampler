MessageHolder::dump() -> String

Thread safety: UNSAFE -- constructs a new String via heap-allocating concatenation.
Returns human-readable event description. Format:
"Type: <type>, Channel: <ch>, Number: <num>, Value: <val>, EventId: <id>, Timestamp: <ts>, "
For PitchBend events, Number/Value/EventId are replaced with a single "Value: <pitchWheelValue>, ".
Also used by the HISE debugger to display MessageHolder objects.

Source:
  ScriptingApiObjects.cpp:5599  ScriptingMessageHolder::dump()
    -> e.getTypeAsString(), e.getChannel(), e.getNoteNumber(), e.getVelocity(),
       e.getEventId(), e.getTimeStamp()
    -> special case: e.isPitchWheel() -> e.getPitchWheelValue()
