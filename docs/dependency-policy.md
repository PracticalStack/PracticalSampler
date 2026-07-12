# Dependency Policy

## Goals

- Fresh-clone builds should not depend on package managers.
- Third-party code should have clear provenance.
- Product-owned code must stay separate from external source trees.
- Upgrades should be deliberate, documented, and reviewable.

## Phase 0 dependency stance

JUCE and HISE are vendored into this repository under `third_party/`.

They are pinned external dependencies, not product-owned code. Vendoring improves reproducibility and avoids setup friction, but it does not make those codebases part of the application's internal architecture.

## Placement

- `third_party/juce/`
- `third_party/hise/`

Note that HISE may itself contain pinned vendored subcomponents or nested upstream snapshots required by its build. Those remain external too and should not be treated as product-owned code merely because they live inside the vendored HISE tree.

## Rules

1. Do not place product-owned source files inside vendored dependency trees.
2. Do not rename vendored directories to look like first-party modules.
3. Record the upstream URL, branch, commit, import date, and any local patch notes for each vendored dependency.
4. Keep local modifications minimal and document them clearly.
5. Route product integration through `engine_adapter/` instead of allowing app code to depend broadly on HISE internals.
6. Prefer updating by importing a fresh upstream snapshot plus documented local patches rather than by making ad hoc edits over time.

## Required provenance record

Each vendored dependency should maintain a small provenance note that includes:

- upstream repository URL
- upstream branch or tag
- pinned commit hash
- import date
- local modifications, if any
- reason for any local modifications

## Out of scope for Phase 0

- automated third-party update tooling
- package-manager-based dependency acquisition
- broad dependency expansion beyond what is needed to stand up the shell and adapter seam
