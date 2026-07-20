# Mini Sprint 6.9 Integration Hardening Contract

Date: July 20, 2026  
Status: Complete

## Closure boundary

Sprint 6 closes only when one explicit typed Publish command is the sole way to replace
Performance. Draft edits, Preview preparation or audition, selection changes, host automation,
failed work, stale work, and editor lifetime changes must preserve the exact active immutable
Performance payload until a complete eligible publication activates at an audio block boundary.

The `PerformancePublishController` owns request/result eligibility, lifecycle truth, and the active
immutable activation payload. Shells issue typed commands and consume immutable presentation
snapshots. The processor adapts the controller-authorized payload to the renderer but does not own a
parallel lifecycle string, macro publication table, or activation-eligibility branch.

## Supported integration budgets

`PerformancePublishIntegrationBudgets` defines the Sprint 6 closure limits:

| Dimension | Limit |
|---|---:|
| Request to active | 8,000,000 microseconds |
| Controller pending depth | 1 |
| Worker pending / in-flight / completed | 2 / 1 / 4 |
| Message-thread service | 250,000 microseconds |
| Retained activation payloads | 64 MiB |
| Retirement backlog | 8 |
| Performance / Preview voices | 24 / 24 |
| Event and note queue drops | 0 |
| Callback overruns / realtime violations | 0 / 0 |

These are supported closure ceilings, not desired UX targets. A change to any ceiling requires fresh
Publish scheduling, mixed-lane integration, offline-render, realtime, load, and Release evidence.

## Integrated proof

`drs.sprint6.integration_hardening` combines authored edits, selected-zone Preview, repeated and
superseding Publish commands, deterministic worker reorder, concurrent host MIDI and automation,
immutable status polling, a held old-generation note, failed-result recovery, activation churn, and
off-audio retirement.

The test retains the active payload before unsaved churn and requires pointer identity and revision
to remain unchanged. It then requires only the newest explicit successful Publish to activate,
proves the held voice keeps its original immutable generation, and drains retirement without
cross-lane note or state leakage.

`drs.sprint6.publish_contract_seams` is the permanent green source audit. It rejects direct shell
facade calls, the former public lifecycle string, duplicate facade macro publication state, and any
loss of controller-owned activation payload or typed command/presentation routing.

## Concurrency ownership

The Linux ThreadSanitizer workflow builds and runs the Sprint 6 integration hardening target beside
the Sprint 4 diagnostics/activation soaks and Sprint 5 integration target. This Windows closure
proves native Debug and Release behavior; ThreadSanitizer execution remains a CI gate.

## Sprint 7 boundary

Sprint 7 receives an explicit, bounded, immutable, last-known-good Performance publication path. It
may add streaming/page pressure, state save/recall and moved-content recovery, device restart,
suspend/resume, and host-session lifecycle behavior. Those features must preserve the Sprint 6
command, identity, atomic activation, generation ownership, macro binding, and Preview-isolation
contracts.
