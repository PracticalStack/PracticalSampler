Content::isCtrlDown() -> Integer

Thread safety: SAFE -- reads static JUCE ModifierKeys state, no allocations or locks.
Returns true (1) if either Ctrl (Windows/Linux) or Command (macOS) is currently pressed.
Cross-platform modifier detection -- checks both Command and Ctrl on macOS.

Source:
  ScriptingApiContent.cpp:8160  Content::isCtrlDown()
    -> ModifierKeys::currentModifiers (static JUCE state)
