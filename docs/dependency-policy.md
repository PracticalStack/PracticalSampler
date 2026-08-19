# Dependency Policy

## Goals

- Fresh-clone builds should not depend on package managers.
- Third-party code should have clear provenance.
- Product-owned code must stay separate from external source trees.
- Upgrades should be deliberate, documented, and reviewable.

## Current dependency stance

JUCE and nlohmann/json are the only vendored third-party dependencies in this repository.

They are pinned external dependencies, not product-owned code. Vendoring improves reproducibility and
avoids setup friction, but it does not make those codebases part of the application's internal
architecture. The product runtime and content contracts are implemented in the product-owned
`engine_adapter/` and `content/` layers.

## Placement

- `third_party/juce/`
- `third_party/nlohmann/json/`

## Rules

1. Do not place product-owned source files inside vendored dependency trees.
2. Do not rename vendored directories to look like first-party modules.
3. Record the upstream URL, branch, commit, import date, and any local patch notes for each vendored dependency.
4. Keep local modifications minimal and document them clearly.
5. Route product integration through `engine_adapter/` instead of allowing app code to depend on
   third-party implementation details.
6. Prefer updating by importing a fresh upstream snapshot plus documented local patches rather than by making ad hoc edits over time.

## Required provenance record

Each vendored dependency should maintain a small provenance note that includes:

- upstream repository URL
- upstream branch or tag
- pinned commit hash
- import date
- local modifications, if any
- reason for any local modifications

## Native runtime boundary

The supported product repository has no authoring-tool dependency. Product code must use the native
runtime models, content-root API, and checked-in `.drsproj`, `.drinst`, `.drstrm`, and `.drpkg`
contracts. The `_analysis/Rhapsody/` tree is reference material and is outside this policy's scope.

## Deferred maintenance

- automated third-party update tooling
- package-manager-based dependency acquisition
- broad dependency expansion beyond what is needed by the native shell and adapter seam
