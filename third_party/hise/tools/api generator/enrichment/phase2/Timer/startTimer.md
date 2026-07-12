## startTimer

**Examples:**

```javascript:debounce-throttle-pattern
// Title: Debounce pattern -- throttle expensive operations
// Context: Prevents rapid re-triggering of an expensive operation (e.g.,
// loading files) by using the running timer as a cooldown gate.

const var processor = Synth.getAudioSampleProcessor("ConvolutionReverb1");

reg shouldReloadOnExpiry = false;

const var cooldownTimer = Engine.createTimerObject();

cooldownTimer.setTimerCallback(function()
{
    this.stopTimer();

    if (shouldReloadOnExpiry)
    {
        shouldReloadOnExpiry = false;
        loadFile();
    }
});

inline function loadFile()
{
    processor.setFile("{PROJECT_FOLDER}impulse.wav");
}

// Called from UI -- may fire rapidly during preset browsing
inline function requestLoad()
{
    if (cooldownTimer.isTimerRunning())
    {
        // Already cooling down -- flag for reload when timer expires
        shouldReloadOnExpiry = true;
        cooldownTimer.startTimer(100); // Reset cooldown
        return;
    }

    // First call: execute immediately, start cooldown
    loadFile();
    cooldownTimer.startTimer(300);
    shouldReloadOnExpiry = false;
}
```
```json:testMetadata:debounce-throttle-pattern
{
  "testable": false,
  "skipReason": "Requires a ConvolutionReverb module and project audio file that cannot be created via API."
}
```

**Pitfalls:**
- Calling `startTimer` on a running timer resets the interval AND resets the internal millisecond counter. If you use `getMilliSecondsSinceCounterReset()` to measure elapsed time independently, a `startTimer` restart silently zeroes your measurement.
