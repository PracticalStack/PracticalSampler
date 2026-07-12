MidiPlayer::connectToPanel(ScriptObject panel) -> undefined

Thread safety: UNSAFE -- Stores a weak reference to the panel object.
Connects this MIDI player to a ScriptPanel for automatic UI updates. The panel receives repaint() calls on sequence changes (always) and on playback position changes (if setRepaintOnPositionChange(true) is also called).
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.connectToPanel(Panel1);
```
Pair with: MidiPlayer.setRepaintOnPositionChange -- enables position-driven repainting on the connected panel.
Anti-patterns: Passing a non-ScriptPanel object throws a script error ("Invalid panel").
Source:
  ScriptingApiObjects.cpp:6250  connectToPanel() -> stores WeakReference<ConstScriptingObject> connectedPanel
