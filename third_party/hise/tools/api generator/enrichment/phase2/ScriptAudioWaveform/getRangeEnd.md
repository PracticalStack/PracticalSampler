## getRangeEnd

**Examples:**

```javascript:envelope-path-from-range
// Title: Convert envelope times to normalized path coordinates
// Context: When overlaying an AHDR envelope shape on a waveform display,
// the total sample count from getRangeEnd() converts millisecond-based
// envelope parameters into normalized (0-1) path coordinates that align
// with the waveform width.

const var wf = Content.addAudioWaveform("Waveform1", 0, 0);
const var overlay = Content.addPanel("EnvelopeOverlay", 0, 0);

inline function rebuildEnvelopePath(attackMs, holdMs, decayMs)
{
    local numSamples = wf.getRangeEnd();

    if (numSamples == 0)
        return;

    local sampleRate = 44100.0;

    // Convert ms envelope times to normalized fractions of the sample length
    local attackNorm = attackMs * 0.001 * sampleRate / numSamples;
    local holdNorm = holdMs * 0.001 * sampleRate / numSamples;
    local decayNorm = decayMs * 0.001 * sampleRate / numSamples;

    local p = Content.createPath();

    p.startNewSubPath(0.0, 0.0);
    p.startNewSubPath(1.0, 1.0);

    // Build envelope shape: silence -> attack -> hold -> decay -> silence
    local x = 0.0;
    p.startNewSubPath(x, 1.0);

    x += attackNorm;
    p.lineTo(Math.min(1.0, x), 0.0);

    x += holdNorm;
    p.lineTo(Math.min(1.0, x), 0.0);

    // Curved decay segment
    p.quadraticTo(Math.min(1.0, x + decayNorm / 3), 1.0,
                  Math.min(1.0, x + decayNorm), 1.0);
    p.lineTo(1.0, 1.0);

    overlay.data.envelopePath = p;
    overlay.repaint();
}

// With no audio loaded, getRangeEnd() returns 0 and the guard skips work
rebuildEnvelopePath(10, 20, 100);
```

```json:testMetadata:envelope-path-from-range
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "wf.getRangeEnd()", "value": 0}
}
```
