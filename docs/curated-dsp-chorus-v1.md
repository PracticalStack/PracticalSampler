# Curated Chorus v1

`drs.chorus` v1 is a fixed three-voice stereo modulation delay. Its surface is `rateHz` (0.05–5),
`depthMs` (0.1–12), `baseDelayMs` (5–30), `width` (0–1), and `mix` (0–1). Voice phases are
permanently spaced 120 degrees apart; the right channel receives a width-controlled quarter-cycle
offset. The phase accumulator advances per sample, so output is deterministic across block sizes
and repeated activation.

The three stereo delay lines are allocated only by `prepare`, have a fixed 42 ms maximum capacity,
and are private to each effect node. Reads use linear interpolation. Chorus has no feedback or
tail, reset clears all line data and returns phase/write positions to zero, and its versioned
catalog metadata reserves the exact fixed voice-state capacity.
