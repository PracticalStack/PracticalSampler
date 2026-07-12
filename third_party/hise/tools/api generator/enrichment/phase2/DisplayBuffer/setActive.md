## setActive

**Examples:**

```javascript:activate-visible-page-only
// Title: Activate display buffers only for the visible page
// Context: A multi-page plugin disables FFT buffers on hidden pages to save CPU.
// When the user switches to a page containing an EQ display, only that page's
// buffer is activated.

const var NUM_CHANNELS = 4;
const var fftBuffers = [];

// Acquire one FFT display buffer per channel EQ
for (i = 0; i < NUM_CHANNELS; i++)
{
    fftBuffers.push(
        Synth.getDisplayBufferSource("EQ " + (i + 1)).getDisplayBuffer(0)
    );
}

// Initially disable all buffers
for (buf in fftBuffers)
    buf.setActive(false);

// When the active channel changes, enable only the selected channel's buffer
inline function onChannelSwitch(activeIndex)
{
    for (i = 0; i < NUM_CHANNELS; i++)
        fftBuffers[i].setActive(i == activeIndex);
};
```
```json:testMetadata:activate-visible-page-only
{
  "testable": false,
  "skipReason": "Requires EQ 1 through EQ 4 modules with display buffer sources"
}
```

```javascript:toggle-spectrum-button
// Title: Toggle spectrum analyser with an enable button
// Context: An EQ module has an optional FFT overlay. The display buffer is
// disabled when the user turns off the spectrum display.

const var fftBuffer = Synth.getDisplayBufferSource("MasterEQ").getDisplayBuffer(0);

inline function onEnableSpectrumControl(component, value)
{
    fftBuffer.setActive(value);
};

Content.getComponent("EnableSpectrum").setControlCallback(onEnableSpectrumControl);
```
```json:testMetadata:toggle-spectrum-button
{
  "testable": false,
  "skipReason": "Requires MasterEQ module with display buffer source and EnableSpectrum UI component"
}
```
