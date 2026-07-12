Message::setAllNotesOffCallback(Function callback) -> undefined

Thread safety: UNSAFE -- creates new WeakCallbackHolder (heap allocation), increments reference count
Registers a callback for AllNotesOff MIDI events. AllNotesOff does NOT trigger onNoteOff
or onController -- this is the only way to receive it. Callback is invoked synchronously
on the audio thread via callSync(), so it must be an inline function.
Callback signature: callback()

Anti-patterns:
  - Using a regular function instead of inline function -- flagged as unsafe in backend
    builds but may silently fail in frontend builds.
  - AllNotesOff events bypass all standard callbacks. If your script needs to reset state
    on AllNotesOff, this callback is the only mechanism.

Source:
  ScriptingApi.cpp  Message::setAllNotesOffCallback()
    -> USE_BACKEND: RealtimeSafetyInfo::check() validates callback
    -> creates WeakCallbackHolder(processor, this, callback, 0)
    -> allNotesOffCallback.incRefCount()
    -> invoked via Message::onAllNotesOff() -> callSync(nullptr, 0, nullptr)
