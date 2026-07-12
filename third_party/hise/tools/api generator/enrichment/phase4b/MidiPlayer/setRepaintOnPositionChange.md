MidiPlayer::setRepaintOnPositionChange(Integer shouldRepaintPanel) -> undefined

Thread safety: UNSAFE -- Starts/stops SuspendableTimer (message thread timer).
When enabled, the connected panel (set via connectToPanel()) receives repaint() calls at 50ms intervals during playback position changes. When disabled, the panel is only repainted on sequence data changes.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.connectToPanel(Panel1);
mp.setRepaintOnPositionChange(true);
```
Pair with: MidiPlayer.connectToPanel -- must be called first to set the target panel.
Source:
  ScriptingApiObjects.cpp:6250  setRepaintOnPositionChange() -> startTimer(50) / stopTimer()
