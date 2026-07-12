## stopBenchmark

**Examples:**

```javascript:measuring-an-init-time-operation
// Title: Measuring an init-time operation
// Context: Wrap a potentially slow initialization step (file I/O,
// waveform processing, module tree construction) to identify
// bottlenecks during development.

Console.startBenchmark();

for (i = 0; i < NUM_SAMPLES; i++)
{
    local af = Engine.createAndRegisterAudioFile(i);
    af.loadFile(sampleFiles[i].toString(0));
}

Console.stopBenchmark(); // Prints e.g. "Benchmark Result: 45.123 ms"

```
