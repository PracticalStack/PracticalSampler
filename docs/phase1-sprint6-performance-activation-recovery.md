# Mini Sprint 6.5 Atomic Performance Activation And Last-Known-Good Recovery

Completed July 20, 2026. This slice makes the Publish controller authorize one exact immutable
Performance payload before renderer staging and requires the same authorization at activation
acknowledgement.

## Authorization boundary

`PerformancePublishActivationPayload` carries a monotonic activation token, complete Publish request
identity, revision, snapshot and prepared build IDs, all five immutable digests, retained bytes, and
the exact `PlaybackActivationPayload` shared owner. The controller independently validates the outer
authorization and retained payload against its current accepted result before entering Pending.

The processor no longer owns a `stagePerformanceActivation()` eligibility branch. It asks the facade
for one controller authorization, builds the render model from that authorization, and stages only
after authorization succeeds. Render-model or slot rejection is reported back as a typed staging
failure. An explicit Publish is never installed through the preparation shortcut; it remains Pending
until the audio callback exchanges the fixed slot at one block boundary.

Startup/default initialization has a typed `bootstrap` request origin. Bootstrap may install
immediately only when no Performance activation exists. It cannot make a creator-issued request
bypass the block-boundary rule.

## Exact acknowledgement and recovery

The processor retains the immutable authorization while its slot is pending. Message service marks
Active only when the Performance context reports the exact authorized revision and prepared build,
and the controller additionally requires the same activation token, snapshot build, digests, and
payload bytes. Repeated, stale, rejected, canceled, or superseded acknowledgements cannot increment
activation count or replace active identity.

Requested, failed, pending, and active identities remain independent. A preparation or staging
failure records the failed request and finding while leaving active request/build/token/digests/bytes
unchanged. Failure clears only when a newer explicit request is accepted; repaired work replaces
last-known-good only after exact acknowledgement.

## Payload lifetime and diagnostics

The Sprint 4 fixed activation slots remain the only callback handoff. Old voices keep their original
immutable model and payload after replacement. The audio side emits bounded retirement tokens only
after the final voice lease ends; message service performs the final shared-owner release.

`SamplerPlaybackContextSnapshot` exposes active, pending, and retired payload bytes, retired backlog,
applied/enqueued/reclaimed counts, and last/maximum reclamation latency in rendered blocks. The
processor’s immutable realtime snapshot carries the combined byte/backlog/count/latency view and the
controller snapshot carries exact active/pending activation tokens, build IDs, bytes, authorization,
staging rejection, acknowledgement rejection, and activation counts.

Pending authorizations are canceled off audio when a newer request, reset, or project close makes
their controller identity stale. Rejected render models are destroyed on the message thread and
never enter an audio-visible slot.

## Deferred boundaries

- Mini Sprint 6.6 proves held/releasing note ownership and stealing across activation generations.
- Mini Sprint 6.7 adds immutable stable-ID published macro value migration.
- Mini Sprint 6.8 replaces direct shell/UI Publish calls and string-only public presentation.
