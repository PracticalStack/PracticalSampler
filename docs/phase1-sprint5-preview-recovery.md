# Mini Sprint 5.6 Last-Known-Good And Recovery Contract

Completed July 19, 2026.

## Outcome

A failed, canceled, superseded, or resource-constrained Preview request cannot replace the active
immutable Preview model. Current, failed, and audible last-known-good request identities remain
independent until a corrected request has prepared and activated successfully.

## Recovery identity

`AuthoringPreviewControllerSnapshot` retains:

- the newest requested identity and preparation/activation state;
- the active request identity and prepared build ID;
- the failed request identity and structured failure finding; and
- bounded completion and warm-result records from the existing controller contract.

New requests clear obsolete failure identity but do not clear the active identity. Cancellation and
supersession retain the active identity. Only `markActive` replaces the last-known-good identity.
Same-project whole-model replacement preserves this controller recovery state; a different project
or explicit Close resets it.

## Stable failure families

Worker, snapshot, preparation, render-model, and activation-slot failures normalize into:

- missing source;
- unsupported format/channel policy;
- invalid range, loop, or offset;
- route/topology conflict;
- decode or content-integrity failure;
- cancellation/supersession;
- bounded resource pressure; or
- internal failure.

Each finding retains stable code, path, message, family, and retryability. Existing creator guidance
continues to consume the formatted code/message while the typed finding remains available to later
status work.

## Audible policy

When a newer request fails and an older activation exists, Preview audition continues on that
last-known-good activation. The status snapshot exposes the failed revision separately and labels
the audible revision as last-known-good. With no active activation, the same note path produces
deterministic silence; it never falls through to Performance.

Old-model voice leases remain unchanged. A repaired source or corrected edit stages normally and
replaces last-known-good only at the audio block boundary. Retired payloads are reclaimed through
the existing message-owned retirement queue.

## Project lifetime

Project Close cancels Preview preparation, resets controller and command ownership, and raises one
primitive close flag. The audio callback consumes that flag at the next block boundary and invokes
the existing bounded Preview context close operation. Performance context, voices, and activation
identity are untouched. Reopen starts a fresh Preview request generation.

