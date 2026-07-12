Sample (object)
Obtain via: Sampler.createSelection(regex), Sampler.createSelectionFromIndexes(index), Sampler.createSelectionWithFilter(function), or Sample.duplicateSample()

Handle to a single sampler sound for reading/writing per-sample properties,
audio data access, and sample map manipulation. Properties are accessed via
integer constants (Sample.Root) or string identifiers ("Root") with bracket syntax.

Constants:
  SampleProperty:
    FileName = 1               Audio file path
    Root = 2                   Root note
    HiKey = 3                  Highest mapped key
    LoKey = 4                  Lowest mapped key
    LoVel = 5                  Lowest mapped velocity
    HiVel = 6                  Highest mapped velocity
    RRGroup = 7                Round-robin group index
    Volume = 8                 Gain in decibels
    Pan = 9                    Stereo panning (-100 to 100)
    Normalized = 10            Enable sample normalization (0/1)
    Pitch = 11                 Pitch factor in cents (+/- 100)
    SampleStart = 12           Start sample offset
    SampleEnd = 13             End sample offset
    SampleStartMod = 14        Sample start modulation range
    LoopStart = 15             Loop start in samples
    LoopEnd = 16               Loop end in samples
    LoopXFade = 17             Loop crossfade length
    LoopEnabled = 18           Enable sample looping (0/1)
    ReleaseStart = 19          Release trigger offset in samples
    LowerVelocityXFade = 20    Lower velocity crossfade length
    UpperVelocityXFade = 21    Upper velocity crossfade length
    SampleState = 22           Sample state (0=Normal, 1=Disabled, 2=Purged)
    Reversed = 23              Play sample in reverse (0/1)

Complexity tiers:
  1. Property read/write: get, set, setFromJSON. Basic sample map editing and
     batch property changes.
  2. Dynamic bounds and metadata: + getRange, getCustomProperties. Query valid
     property ranges before setting loop points; attach transient analysis data.
  3. Audio manipulation: + loadIntoBufferArray, replaceAudioFile, deleteSample,
     duplicateSample. Load audio for analysis, write modified audio back,
     restructure sample maps programmatically.

Practical defaults:
  - Use Sampler.createSelection(".*") or createSelectionFromIndexes(-1) to get
    all samples as Sample objects.
  - When setting velocity ranges, set HiVel first when widening, LoVel first
    when narrowing -- auto-clipping clamps against the current opposite bound.
  - Use getRange() before setting loop-related properties. Loop bounds are
    interdependent (LoopEnd depends on SampleEnd, LoopStart depends on LoopXFade).
  - Attach analysis results to getCustomProperties() rather than parallel arrays --
    metadata stays associated through sorting operations.

Common mistakes:
  - Setting LoVel/HiVel without considering order -- auto-clipping clamps values
    against the current opposite bound. Set HiVel first when widening, LoVel first
    when narrowing.
  - Setting loop points without checking getRange() first -- loop ranges are
    dynamic and values outside valid bounds are silently clamped.
  - Using a Sample reference after deleteSample() -- the underlying sound is
    removed; further method calls throw "Sound does not exist".
  - Calling replaceAudioFile() on monolithic samples -- throws "Can't write to
    monolith files". Only works with non-monolithic sample files.
  - Storing analysis data in parallel arrays instead of getCustomProperties() --
    parallel arrays break when selections are sorted or filtered.

Example:
  // Get all samples from the sampler
  const var allSamples = Sampler.createSelectionFromIndexes(-1);

  // Access a single sample
  const var s = allSamples[0];

  // Read and modify properties
  var root = s.get(Sample.Root);
  s.set(Sample.Root, 60);

  // Bracket syntax also works
  s[Sample.Volume] = -6;

Methods (9):
  deleteSample          duplicateSample       get
  getCustomProperties   loadIntoBufferArray   refersToSameSample
  replaceAudioFile      set                   setFromJSON
