# Curated DSP schema fixtures

These compact fixtures are parsed with referenced-path validation disabled. They isolate authored
DSP topology and are shared by loader and host-state structural tests.

| Fixture | Expected result | Coverage |
|---|---|---|
| `valid-all-scopes.json` | valid | zone, group, master ownership; known and unknown preserved slot |
| `negative-cases.json` | case definitions | unknown version, missing/duplicate parameter, shared/orphan slot, duplicate source owner, and budget cap |

`negative-cases.json` applies its deterministic mutations to the all-scopes project, keeping each
negative case focused without duplicating an otherwise identical manifest. The loader contract suite
executes every case; the host-state suite embeds the all-scopes project and enforces the same
1,024-parameter ceiling.
