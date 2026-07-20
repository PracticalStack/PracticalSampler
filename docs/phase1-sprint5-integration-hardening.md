# Mini Sprint 5.8 Integration Hardening Contract

Date: July 19, 2026  
Status: Complete

## Closure boundary

Sprint 5 closes only when authored Preview work remains bounded and isolated while concurrent
Performance audio continues to render the exact published immutable payload. Preview may prepare,
activate, audition, fail, recover, close, and reopen without publishing a draft or changing the
Performance revision, build identity, content digest, or offline output.

The authoring shell sends only typed `AuthoringPreviewCommand` values. It does not own duplicate
note-on/note-off callbacks, preparation branches, lifecycle strings, or failure identities. The
controller's immutable snapshot is the authority for Preview lifecycle and failure presentation.

## Supported integration budgets

`AuthoringPreviewIntegrationBudgets` defines the closure limits:

| Dimension | Limit |
|---|---:|
| Absolute coalescing delay | 40,000 microseconds |
| Request to audible | 8,000,000 microseconds |
| Controller pending depth | 1 |
| Worker pending work | 2 |
| Worker in-flight work | 1 |
| Retained activation payloads | 64 MiB |
| Retirement backlog | 8 |
| Event/note queue drops | 0 |
| Callback overruns / realtime violations | 0 |

These values are support limits rather than performance targets. Changing one requires fresh
integration, realtime, shell-parity, offline-conformance, and Release evidence.

## Integrated proof

`drs.sprint5.integration_hardening` exercises mixed selection, mapping, gain, pan, root-key,
audition, and stop commands while a separate thread renders Performance. It also covers immutable
status polling, newest-wins worker completion ordering, repeated Preview activation and retirement,
project close/reopen, and last-known-good activation lifetime.

Before Preview churn, the test retains the published Performance payload and renders an offline
baseline. After unsaved churn it requires pointer identity, revision, prepared build ID, prepared
content digest, and sample output to remain unchanged. The test then closes and reopens the project
and requires Preview to clear and recover without advancing the published Performance revision.

The integration soak uses the supported 1024-sample callback profile to avoid classifying an
involuntary desktop/CI scheduler preemption as sampler work. The dedicated realtime matrix remains
authoritative across every supported 44.1/48 kHz and 32-1024 sample callback profile.

## Concurrency ownership

The Linux ThreadSanitizer workflow builds and runs the Sprint 5 integration hardening target beside
the Sprint 4 diagnostics-concurrency and activation-retirement soak targets. Windows closure proves
the native Debug and Release configurations; ThreadSanitizer remains a CI gate rather than a local
Windows claim.

## Sprint 6 boundary

Sprint 5 deliberately does not define Apply/Publish behavior. Sprint 6 receives the immutable
render model, typed request/result pattern, last-known-good Preview behavior, block-boundary
activation, off-audio retirement, and immutable status publication. Sprint 6 must separately decide
the explicit Publish command, full-project Performance request, failed-publish recovery, held-note
cutover, and automation/macro transfer.
