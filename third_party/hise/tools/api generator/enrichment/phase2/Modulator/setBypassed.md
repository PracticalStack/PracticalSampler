## setBypassed

**Examples:**

```javascript:toggle-key-follow
// Title: Toggle filter key-follow from a button
// Context: A common pattern is using bypass to enable/disable optional modulators
// based on UI toggle buttons. The negation (!value) maps button on -> unbypassed.

const var filterKeyMod = Synth.getModulator("FilterKeyFollow1");

inline function onKeyFollowToggle(component, value)
{
    filterKeyMod.setBypassed(!value);
}

keyFollowButton.setControlCallback(onKeyFollowToggle);
```
```json:testMetadata:toggle-key-follow
{
  "testable": false,
  "skipReason": "Requires a FilterKeyFollow modulator and UI button in the module tree"
}
```

```javascript:stereo-midside-switch
// Title: Switch EQ between stereo and mid-side mode
// Context: Mode switching often requires bypassing one set of modules while
// activating another. This pattern uses a single index to toggle between
// two processing paths.

const var msEncoder = Synth.getEffect("MSEncoder1");
const var msDecoder = Synth.getEffect("MSDecoder1");
const var stereoEQ = Synth.getEffect("StereoEQ");
const var midEQ = Synth.getEffect("MidEQ");
const var sideEQ = Synth.getEffect("SideEQ");

inline function onEqModeControl(component, value)
{
    local isMidSide = parseInt(value) == 1;

    msEncoder.setBypassed(!isMidSide);
    msDecoder.setBypassed(!isMidSide);
    stereoEQ.setBypassed(isMidSide);
    midEQ.setBypassed(!isMidSide);
    sideEQ.setBypassed(!isMidSide);
}

eqModeSelector.setControlCallback(onEqModeControl);
```
```json:testMetadata:stereo-midside-switch
{
  "testable": false,
  "skipReason": "Requires multiple effect modules and a UI selector in the module tree"
}
```
