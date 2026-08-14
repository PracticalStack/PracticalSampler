# Playable Instrument License LI-01 Contract

Status: complete on 2026-08-14. Production behavior begins in LI-02.

## Frozen contract

| Concern | Contract |
| --- | --- |
| Project import menu | `Import License File...` |
| Performance menu | `View License` |
| Project-root filename | `LICENSE.txt` |
| Package payload id | `license-text` |
| Package logical path | `LICENSE.txt` |
| Media type | `text/plain; charset=utf-8` |
| Maximum size | 1 MiB (1,048,576 bytes) |
| Encoding | Valid UTF-8; a UTF-8 BOM is accepted |
| Unsafe content | Embedded NUL bytes are rejected |
| Byte handling | Accepted source bytes are preserved exactly |
| Manifest compatibility | Optional additive member; no package schema bump |
| Package-v2 record kind | `7` (`licenseText`) |

The canonical constants live in
`engine_adapter/include/drs/engine/PlayableInstrumentLicense.h`. Menu labels live
in `app/src/shared/WorkspaceMenuPolicy.h`.

## Compatibility decision

The license is optional metadata and does not change synthesis, routing, or the
runtime instrument schema. Existing schema-1 and schema-2 packages therefore
retain their current version numbers. A missing license member means that the
package has no license payload. Readers implemented in LI-03 must fail closed if
a package declares a license payload that is missing, corrupt, oversized, or not
valid UTF-8.

## Expected-red seams

`drs_playable_instrument_license_contract_red_tests` is intentionally excluded
from CTest and `drs_all_tests`. Direct invocation currently returns exit code 1
for each named seam:

- `project-import-storage`
- `package-export-persistence`
- `package-reader-integrity`
- `activation-ownership`
- `plugin-menu-viewer`
- `standalone-menu-viewer`

An exit code of 1 means the named production behavior is still absent as
expected. Each later slice must replace its source audit with behavioral green
coverage before the feature is considered delivered. Exit code 2 indicates a
test setup or invocation failure and is never an expected result.

## LI-01 evidence

- `drs.playable_instrument_license.contract` passes.
- All six direct expected-red seams return exit code 1.
- `drs.package_v2.records` passes.
- `drs.phase1.performance_package` passes.
- `drs.performance_package.export_lifecycle` passes.

