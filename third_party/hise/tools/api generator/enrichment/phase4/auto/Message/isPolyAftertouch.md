Returns whether the current event is a polyphonic aftertouch message. Use inside `onController` before calling `Message.getPolyAfterTouchNoteNumber()` and `Message.getPolyAfterTouchPressureValue()`.

> [!Warning:Returns true for both aftertouch types] Returns true for both polyphonic aftertouch and channel pressure events, since HISE uses the same internal event type for both. See `Message.isMonophonicAfterTouch()` for details on distinguishing them.
