# Phase 1 Draft-To-Playback Sprint 1 Contract

This note turns the Sprint 1 behavior bullets from section 6.1 of `engineering-plan.html` into an explicit product-owned contract.

The current implementation target is the lightweight `DraftPlaybackContract` state machine in `engine_adapter`. It does not render audio or prepare assets. Its purpose is to freeze the revision and ownership rules before later sprints wire real snapshot building, prepared assets, and activation into the shell.

## What Sprint 1 freezes

- the authoring document owns the mutable draft revision
- Preview can track the newest successfully prepared draft revision without changing Performance
- Performance changes only after a successful Apply or Publish completion
- failed or canceled preparation keeps the last known-good published revision active
- closing the project unloads both playback paths and cancels pending work
- device restart preserves the published revision identity but temporarily moves playback paths into a restarting state

## Behavior matrix

- Draft edit: increments `draftRevision` and can make Preview stale while Performance stays on the last applied revision.
- Preview success: updates only the Preview revision and marks it `Ready` when it matches the current draft.
- Preview failure: retains the last good Preview revision when one exists and surfaces actionable issues.
- Publish success: updates only the Performance revision and only after the requested build completes.
- Publish cancel: clears pending publish work without changing the active Performance revision.
- Publish supersede: an older pending publish request cannot activate after a newer request replaces it.
- Publish failure: keeps the last known-good Performance revision active and records the failure issues.
- Close project: clears Preview and Performance availability plus all pending work.
- Device restart: preserves revision identities, rejects new build requests while restarting, and restores the last active published revision after restart recovery.

## Ownership rules

- Message thread: owns authoring edits, revision changes, request dispatch, cancel, close, and restart transitions.
- Preparation workers: own future snapshot build results, success, failure, and supersede callbacks. Sprint 1 models these as explicit completion methods instead of hidden side effects.
- Audio thread: owns neither draft mutation nor request orchestration. Sprint 1 keeps the audio-thread boundary implicit by forbidding draft and publish state changes except through prepared completion events.

## Executable coverage

`tests/src/Phase1DraftPlaybackContractTests.cpp` now exercises:

- draft revision advancement through the Phase 2 authoring document controller
- Preview revision drift and recovery
- Apply or Publish success, cancel, failure, and supersede behavior
- close-project unload behavior
- device-restart preservation of the last published revision identity

This is intentionally a Sprint 1 seam. Sprint 2 can replace the synthetic completion calls with real immutable snapshot build results without redefining these user-visible rules.
