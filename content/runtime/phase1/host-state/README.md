# Host-State Contract Fixtures

These fixtures freeze the `drs.hostState` version 1 contract defined by
`docs/host-state-recall-adr.md`.

| Fixture | Classification | Purpose |
|---|---|---|
| `reference/saved-project.hoststate.json` | valid | Clean saved project with every optional binding and published-identity field |
| `reference/dirty-project.hoststate.json` | valid | Dirty never-saved project with a bounded embedded project snapshot |
| `legacy/lead-performance.preset-state.json` | legacy valid | Raw `drs.presetState` v1 migration input |
| `negative/unknown-version.hoststate.json` | invalid | Supported schema name with unsupported major version |
| `negative/corrupt.hoststate.json` | invalid | Truncated JSON |
| `negative/over-budget.fixture.json` | generated invalid | Deterministic recipe for synthesizing a payload one byte over the 8 MiB limit |
| `negative/identity-mismatch.hoststate.json` | invalid | Binding project ID differs from the embedded project ID |

## Field coverage

`saved-project.hoststate.json` includes all required top-level fields, every `projectBinding`
field, the complete clean `authoringState`, the complete `publishedState`, and a complete nested
preset.

`dirty-project.hoststate.json` adds the conditional `projectSnapshot` field. Its snapshot includes
all required project and authoring collections while intentionally containing no sample bytes.

`over-budget.fixture.json` is a compact generator descriptor rather than an 8 MiB checked-in blob.
The contract test reads `baseFixture`, adds a top-level unknown `padding` string, and grows it until
the complete serialized input is exactly `targetUtf8Bytes`. It asserts rejection occurs on the
total byte budget before JSON field validation. This keeps repository size bounded while defining
the over-budget input exactly.

All reference JSON uses UTF-8, LF line endings, two-space indentation, and a final newline.
