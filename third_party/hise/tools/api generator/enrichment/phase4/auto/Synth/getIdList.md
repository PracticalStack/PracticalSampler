Returns an array of processor IDs for all processors of the given type within the parent synth's subtree. The `type` parameter is the processor's type name (e.g. `"LFO Modulator"`, `"Simple Gain"`), not a user-assigned processor ID.

> [!Warning:Pass type name, not processor ID] Passing a processor ID like `"MyLFO"` instead of the type name `"LFO Modulator"` returns an empty array with no error, making the mistake hard to diagnose.
