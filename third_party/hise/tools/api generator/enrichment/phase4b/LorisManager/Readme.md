LorisManager (object)
Obtain via: Engine.getLorisManager()

Loris partial-tracking library interface for spectral analysis, per-partial
manipulation, and resynthesis of audio files. Requires HISE_INCLUDE_LORIS
compile flag. All wrappers share a singleton LorisManager from MainController.

Common mistakes:
  - Passing a string path instead of a File object to analyse/process/synthesise
    -- silently returns false/empty with no error. Use FileSystem.fromAbsolutePath().
  - Passing a plain number as the data argument to process() instead of a JSON
    object -- e.g. process(f, "shiftPitch", 2.0) instead of
    process(f, "shiftPitch", {"offset": 2.0}).

Example:
  const lm = Engine.getLorisManager();
  const f = FileSystem.fromAbsolutePath("path/to/sample.wav");

  lm.set("timedomain", "seconds");
  lm.analyse(f, 440.0);
  lm.process(f, "shiftPitch", {"offset": 1.5});
  const buffers = lm.synthesise(f);

Methods (9):
  analyse              createEnvelopePaths  createEnvelopes
  createSnapshot       get                  process
  processCustom        set                  synthesise
