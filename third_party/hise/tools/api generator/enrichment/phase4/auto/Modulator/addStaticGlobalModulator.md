Creates a static global modulator receiver that samples the source value once at voice start rather than continuously tracking it. This is more CPU-efficient than `addGlobalModulator` for sources that only produce a value at note-on, such as velocity, key number, or random modulators.

> [!Warning:Captures single snapshot per note] Using this with a continuously changing source (LFO, envelope) captures a single snapshot per note - the modulation will not update during the note's lifetime.
