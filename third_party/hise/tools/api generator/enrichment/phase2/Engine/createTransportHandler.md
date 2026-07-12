## createTransportHandler

**Examples:**

```javascript:transport-handler-broadcaster-ui
// Title: Transport handler driving UI buttons through a broadcaster
// Context: Sequencer-based plugins need to sync play/stop state with
// the host DAW. This pattern creates a transport handler, connects it
// to a broadcaster, and lets the broadcaster update UI components.
// --- setup ---
Content.addButton("PlayButton", 0, 0);
// --- end setup ---

const var transportHandler = Engine.createTransportHandler();

const var transportBroadcaster = Engine.createBroadcaster({
    "id": "Transport Broadcaster",
    "args": ["isPlaying"]
});

// Connect transport changes to the broadcaster.
// false = don't send the current state immediately
transportHandler.setOnTransportChange(false, transportBroadcaster);

// Enable grid quantization at 1/8 note resolution (index 8)
transportHandler.setEnableGrid(true, 8);

// Prefer internal clock but sync to host when available
transportHandler.setSyncMode(transportHandler.PreferInternal);

// Stop the internal clock when the host stops
transportHandler.stopInternalClockOnExternalStop(true);

// The broadcaster updates the play button when transport state changes
const var playButton = Content.getComponent("PlayButton");

transportBroadcaster.addListener(playButton, "sync play button", function(isPlaying)
{
    this.setValue(isPlaying);
});
```
```json:testMetadata:transport-handler-broadcaster-ui
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "typeof transportHandler", "value": "object"},
    {"type": "REPL", "expression": "transportHandler.isPlaying()", "value": false}
  ]
}
```
