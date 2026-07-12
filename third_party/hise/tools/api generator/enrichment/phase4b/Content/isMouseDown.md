Content::isMouseDown() -> Integer

Thread safety: SAFE -- reads JUCE Desktop mouse source modifiers, no allocations or locks.
Returns the current mouse button state: 0 if no button is pressed, 1 if left button
is down, 2 if right button is down.

Source:
  ScriptingApiContent.cpp:9036  Content::isMouseDown()
    -> Desktop mouse source -> ModifierKeys check
    -> returns 0 (none), 1 (left), 2 (right)
