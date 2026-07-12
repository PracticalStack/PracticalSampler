Message::isMonophonicAfterTouch() -> Integer

Thread safety: SAFE
Returns whether the current event is a monophonic (channel pressure) aftertouch message.
Used inside onController to distinguish before calling getMonophonicAftertouchPressure().

Anti-patterns:
  - Both isMonophonicAfterTouch() and isPolyAftertouch() check the same underlying
    Type::Aftertouch enum. They both return true for ANY aftertouch event. The HISE event
    system does not distinguish mono vs poly aftertouch at the type level. In practice,
    channel pressure events typically have note number 0 while poly aftertouch carries
    the target note number.

Source:
  ScriptingApi.cpp  Message::isMonophonicAfterTouch()
    -> constMessageHolder->isChannelPressure()
    -> checks type == Type::Aftertouch (same as isAftertouch)
