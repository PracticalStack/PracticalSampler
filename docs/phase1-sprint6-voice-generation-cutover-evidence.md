# Mini Sprint 6.6 Completion Evidence

Date: July 20, 2026  
Decision: Pass

## Implemented evidence

- Added one explicit non-zero activation generation to the fixed Performance activation exchange.
- Bound every started voice to its generation and immutable render model until finish/reset.
- Preserved old routes, decoded sample handles, pitch, loop, gain, and release behavior across
  activation.
- Added sustain-pedal events and host CC64 routing; sustain release targets deferred voices across
  generations without rebinding them.
- Split host CC123 into all-notes-off release and CC120 into immediate all-sound-off/reset.
- Defined fixed Performance pressure order as retired-active, current-active, retired-releasing,
  then current-releasing, with generation/voice identity tie breaks.
- Added generation, sustain, and steal diagnostics through the context and processor snapshots.

## Dedicated matrix

`drs.sprint6.voice_generation_cutover` proves:

- a held old route and a sample-zero retrigger have distinct, exact old/new generation owners;
- removed routes and changed pitch, loop, gain, and sample handles remain immutable per voice;
- sustain defers and releases the correct old-generation voice;
- decoded ownership survives the held voice and release tail, then reclaims off audio;
- all-notes-off and reset address all Performance generations without touching Preview;
- 24-voice pressure remains bounded and steals an old active voice before a release tail;
- cross-generation and release-tail steal counters identify the selected policy;
- four simultaneous leased generations are accepted, a fifth is rejected, and reclamation permits
  the newest generation afterward; and
- host CC64 and CC123 reach the Performance-only generation path.

## Validation results

| Check | Result |
| --- | --- |
| Debug `drs_all_tests` aggregate | Passed; 67 affected compile/link steps completed after the 6.6 target was registered. |
| CTest discovery | Passed; 66 tests discovered and 6.6 is registered as test 37. |
| Sprint 4 renderer/voice/context cluster | Passed 7/7 in 8.19 seconds. |
| Sprint 6 contract through generation cutover | Passed 7/7 in 9.29 seconds. |
| Dedicated 6.6 matrix | Passed through CTest in 1.84 seconds and directly after the aggregate relink. |
| Realtime safety and strict entry guard | Safety passed through CTest in 18.22 seconds and directly in 19.6 seconds; the guard passed in 33.95 seconds. |
| Concurrency soak | Passed directly: 5,032 blocks, more than 2.5 million coherent polls, three activations, one reclaimed payload, and zero violations. |
| Sprint 5 integration hardening | Passed directly with zero dropped events/notes, over-budget callbacks, or realtime violations. |
| Expected-red seam audit | Expected failure with exactly five later-sprint seams. |

One parallel/direct JUCE batch and the first isolated realtime run stalled in the known Windows
process runner. Each affected executable subsequently passed alone; the authoritative CTest
clusters above are green.

## Exit assessment

Mini Sprint 6.6 exit criteria are met: every live voice has exactly one non-zero activation
generation and one immutable model owner until cleanup. Held/releasing voices cannot dangle,
new notes cannot route backward, fixed capacity cannot grow, and Performance events cannot mutate
Preview ownership. Mini Sprint 6.7 may proceed with published macro and automation revision binding.
