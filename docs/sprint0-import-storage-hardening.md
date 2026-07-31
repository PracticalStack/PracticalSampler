# Sprint 0 Import And Storage Hardening

Implemented July 31, 2026.

This foundation pass makes hostile or unusually large SFZ input bounded and makes the paired
project-manifest save recoverable. It deliberately changes infrastructure behavior without
claiming broader opcode conversion support.

## SFZ safety budgets

Every `SfzImportExecutionContext` carries explicit limits. The defaults are:

- 16 MiB of source text across the expanded document
- 256 include expansions
- 32 levels of include depth
- 100,000 sections
- 65,536 regions
- 512 retained findings

Crossing a structural budget stops parsing with a blocking `budget.*_exceeded` finding and the
`budgetExceeded` failure reason. Findings beyond the retention limit are counted in
`suppressedFindingCount`; analysis and review decisions continue to use structural opcode counts,
so a capped finding vector cannot accidentally make blocking content importable.

The review UI groups findings by severity, disposition, code, and summary. It renders at most 100
groups and reports both additional groups and engine-suppressed findings instead of constructing a
text editor containing thousands of repeated lines.

## SFZ preprocessing and paths

The parser now implements textual `$name` substitution for `#define $name value`. Definitions are
visible to later lines and recursively included files, matching the use in the SM Drums programs
for MIDI key and CC-number remapping. Macro expansion applies to opcode names, opcode values, and
include paths. Undefined and malformed macros are blocking parser findings.

Unquoted opcode values may contain spaces. Parsing stops a value only at the next recognizable
`opcode = value` assignment or section header, so labels and sample filenames keep their embedded
spaces.

`#include` remains relative to the file containing the directive. In contrast, relative `sample=`
paths use the top-level SFZ directory as their base, including samples declared by included mapping
files. `prefix_sfz_path` is applied on top of that base. This is the layout used by SM Drums.

## Recoverable project-pair saves

Saving a `.drsproj` and its matching `.drinst` is now a journaled two-file transaction:

1. Serialize both files into temporary files.
2. Back up the complete previous generation.
3. Atomically publish a small sibling save journal.
4. Replace the instrument and project targets.
5. Remove the journal, then remove the backups.

Any commit failure invokes rollback immediately. If the process ends between steps, the next save,
interactive open, or plug-in project-binding validation sees the journal and restores both members
of the previous generation before loading. Backups remain present until both restorations succeed,
so recovery itself is retryable.

## Permanent regression coverage

`drs.sprint0.import_storage_hardening` covers:

- every SFZ budget and retained-finding suppression
- macro expansion in names and values
- whitespace-bearing labels and sample paths
- include-relative includes with root-relative samples
- the full `SM_Drums_kit.sfz` corpus: 31 source files and 3,358 regions
- bounded SM Drums diagnostics and successful sample resolution
- rollback at the instrument/project commit boundary
- next-open recovery from a persisted interrupted-save journal

The existing parser, review UI, and determinism suites also exercise the new behavior.
