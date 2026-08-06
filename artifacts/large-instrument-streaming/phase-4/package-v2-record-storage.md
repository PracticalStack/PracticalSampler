# Phase 4 package v2 record storage

Date: 2026-08-05

## Result

STR-400 through STR-403 and every Phase 4 acceptance criterion pass.

## Binary and authentication contract

- Magic `DRSPKG2`, format version 2.
- Packed fixed-size header and TOC entries use unsigned 64-bit offsets, sealed/plaintext sizes, and page indices.
- Record identity is `(sourceId, recordKind, pageIndex)`; duplicates reject.
- Manifest, runtime instrument, stream index, sample head, sample page, and background records are independently addressable.
- Each record is limited to 64 KiB plaintext. The writer predicts and verifies the provider's bounded sealed size before writing.
- AAD binds format, package id, source id, record kind, page index, plaintext size, and plaintext checksum. TOC identity substitution therefore fails authentication.
- The reader opens only the fixed header and TOC. Record open seeks to one checked range, authenticates it, then independently checks size and checksum before returning bytes.

## Qualification matrix

- Six-record package: metadata-only open, exact page 0 range read, no unrelated record opens.
- Corrupted page 1: authentication failure localized to page 1; page 0 still opens.
- Authenticated-but-sabotaged plaintext: checksum failure and no published bytes.
- Cancellation before I/O: zero bytes read and precise cancellation count.
- Truncated record region: checked-bounds failure.
- Duplicate identity and 65,537-byte record: deterministic policy rejection.
- Sparse package: record offset `5 * 1024^3`, file size above 32-bit range, and page index 7,000,000,000 survive metadata/TOC open without payload allocation.

## Package-backed sampler path

`PackagePagedSampleDataSource` retains the immutable package open result (path plus TOC) for its activation lifetime, maps head/page requests to record identities, and publishes float views only after record authentication and checksum. The production `SamplerVoice` renders continuously across the package head/page-0 boundary. Cancellation and corrupted page 1 publish nothing and leave the prior ready page valid. Primitive look-ahead intents drain through the common worker scheduler.

## Compatibility dispatch

- `DRSPKG2` uses the bounded v2 reader.
- `DRSPKG1` loads through the deterministic legacy reader only when the package is at most 64 MiB.
- Larger v1 files reject before whole-file legacy loading with explicit v2 re-export guidance.
- Tests compare size and modification time across a small v1 open and verify no rewrite-on-load behavior.

## Verification

```powershell
cmake --build --preset build-debug --target drs_package_v2_tests
build\vs2022-debug\tests\drs_package_v2_tests.exe
```

Result: `Package v2 bounded record matrix passed.`
