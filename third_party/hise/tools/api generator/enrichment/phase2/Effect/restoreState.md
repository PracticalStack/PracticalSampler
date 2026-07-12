## restoreState

**Examples:**

```javascript:restore-and-refresh-ui
// Title: Restore effect state and force UI notification refresh
// Context: After calling restoreState(), connected UI components do not
// automatically update. Re-set parameters to trigger notifications,
// or call updateValueFromProcessorConnection() on connected components.

const var fx = Synth.getEffect("MasterComp");
const var fxControls = [Content.getComponent("CompAttack"),
                        Content.getComponent("CompRelease"),
                        Content.getComponent("CompMix")];

inline function restoreAndRefreshUI(savedState)
{
    fx.restoreState(savedState);

    // Resync UI components that are connected to this processor
    for (c in fxControls)
        c.updateValueFromProcessorConnection();
}
```
```json:testMetadata:restore-and-refresh-ui
{
  "testable": false,
  "skipReason": "Requires UI components connected to a processor via processorConnection and a valid Base64 state string from a prior exportState() call"
}
```

**Pitfalls:**
- [BUG] `restoreState()` does not fire attribute change notifications. If you have Broadcasters attached via `attachToModuleParameter()`, they will not fire after a `restoreState()` call. Work around this by re-setting the parameter value: `fx.setAttribute(fx.Param, fx.getAttribute(fx.Param))` to trigger the notification chain.
