# Mini Sprint 5.2 Preview Controller Completion Evidence

Completed July 19, 2026.

## Outcome

Preview lifecycle and newest-result acceptance are now owned by `AuthoringPreviewController`.
Authoring revision and selection observations produce typed requests, exact duplicate observations
produce no work, selection/revision changes supersede older requests, and only the full current
identity can become Ready or Active.

## Implemented artifacts

- `AuthoringPreviewRequest` adds reason and deterministic signature to the frozen request identity.
- `AuthoringPreviewController` implements request issuance, eligibility, duplicate suppression,
  supersession, cancellation generations, legal transitions, newest-result acceptance, activation
  reconciliation, reset, failures, and counters.
- `PluginProcessor` now observes AuthoringSession state, creates controller requests, services the
  existing worker facade, and stages accepted models through the isolated Preview playback context.
- The old `synchronizeAuthoringPreviewActivation` revision/selection/build inference path and its
  processor-owned observation fields were removed.
- `drs.sprint5.preview_controller` and
  `drs.sprint5.preview_controller_integration` are registered with CTest and `drs_all_tests`.

## Validation

| Validation | Result |
| --- | --- |
| Focused build | Passed for contract, controller, processor integration, and expected-red audit targets. |
| Sprint 5 contract/controller CTest matrix | **Passed 3/3** in 2.01 s on the final focused run. |
| Controller identity matrix | Rejects independent request-ID, revision, scope, selection, and cancellation-generation mismatches. |
| Processor integration | Project open, selection replacement, immutable staging, block-boundary activation, duplicate suppression, and Preview/Performance isolation passed. |
| Direct expected-red audit | Expected exit 1; reports exactly 4 remaining seams, down from 5 after lifecycle synchronizer removal. |
| Inherited Sprint 4/entry/realtime matrix | **Passed 14/14**; first 11 completed before the command time limit and the remaining 3 passed in 78.67 s. |

## Deferred seams remain visible

The direct-only red audit still records processor immediate payload construction, synchronous sample
warming, implicit message servicing from Preview note-on, and string-only public lifecycle state.
Their planned owners remain Mini Sprints 5.4, 5.5, and 5.7. The red target remains outside CTest and
the aggregate until those seams are removed.

## Exit decision

Mini Sprint 5.2 exit criteria are met. One typed controller owns request identity, lifecycle, result
eligibility, and activation state. Older or mismatched work cannot overwrite newer controller state
or become Ready/Active. Mini Sprint 5.3 may proceed with bounded coalescing, supersession metrics,
and worker cancellation.
