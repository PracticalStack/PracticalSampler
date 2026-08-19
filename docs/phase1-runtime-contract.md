# Phase 1 Runtime Contract

This note defines the first product-owned runtime-format subset for Sprint 1 of Phase 1. The point of this contract is not to finalize the shipping format. It is to stop runtime work from depending on vague assumptions.

## Sprint 1 goals

- establish versioned product-owned runtime object identities
- define the minimum shape of `.drsproj`, `.drinst`, and `.drstrm`
- give the engine adapter a real manifest target to load
- create golden fixtures that tests can validate before the import compiler exists

## Temporary encoding choice

During Sprint 1, the native runtime files use their final extensions but are encoded as JSON text fixtures:

- `.drsproj`
- `.drinst`
- `.drstrm`

This is intentional. It lets the team review the contract in source control and validate parsing behavior before Sprint 2 introduces compiler-emitted artifacts and a more compact on-disk representation.

## Schema rules

### Common rules

- every file declares `schemaName`
- every file declares `schemaVersion`
- identifiers are stable product-owned strings, not third-party object names
- relative asset paths are resolved from the containing manifest file
- unknown future fields should be ignored unless they alter safety-critical load behavior

### `.drsproj`

Sprint 1 project fixtures carry:

- project identity
- display name
- content-root hint
- default instrument manifest path
- source sample references

Sprint 1 does not require the loader to build directly from `.drsproj`, but the project file must exist so the compiled manifest can point back to a product-owned source artifact.

### `.drinst`

Sprint 1 instrument manifests are the main loader target. They must describe:

- instrument identity and display name
- source project path
- compiled stream asset path
- default load profile
- macro definitions
- articulation definitions
- group definitions
- zone definitions
- validation notes

Each zone must include:

- stable zone id
- group id
- articulation id
- sample path
- stream asset path
- root, key, and velocity ranges
- stream offset bytes
- prefetch bytes

### `.drstrm`

Sprint 2 now emits a prototype stream-container descriptor so the loader and compiler tests can validate that a stream asset exists, is referenced explicitly, and carries deterministic paging metadata.

The prototype descriptor currently establishes:

- stream container identity
- schema version
- page size intent
- sample count intent
- payload encoding
- per-sample source metadata
- payload offsets and sizes
- page-table placeholder entries beyond the prefetch head

## Product-owned runtime model

The adapter-side runtime model now has explicit product-owned structs for:

- macros
- articulations
- groups
- zones
- instrument manifest
- stream-container samples and pages
- load metrics
- manifest load results
- stream-container load and read-resolution results

That model is intentionally defined in `engine_adapter/include/drs/engine/RuntimeModel.h` so the
shell can consume runtime status without reaching into third-party types.

## Loader contract

The Sprint 1 loader must:

- locate the Phase 1 reference manifest
- parse the JSON fixture
- validate required fields
- resolve relative paths
- verify referenced files exist
- build an in-memory runtime instrument model
- report counts and prefetch totals
- surface issues without crashing the shell

Sprint 1 now also requires deterministic serialization back to the checked-in golden files for:

- `.drsproj`
- `.drinst`

The Sprint 1 loader does not yet:

- decode sample data
- allocate voices
- page stream data
- compile source assets into runtime artifacts

Sprint 3 now adds the first product-owned `.drstrm` reader, which must:

- parse the compiled stream descriptor
- validate source checksums plus payload and page-table layout
- resolve sample-relative offsets into either prefetch-head or page-table spans

Sprint 3 now also adds the first product-owned background streaming service, which must:

- accept page-read requests without blocking the requester thread
- resolve queued reads on a worker thread
- expose resident-page, pending-request, and lease-lifetime state explicitly
- expose the active load-profile id plus configured cache budget explicitly

Sprint 3 now also adds the first product-owned voice state object, which must:

- bind a concrete zone, group, articulation, and macro snapshot at allocation time
- advance explicit stream cursors through head data and streamed pages
- wait, resume, release, and finish without stale lease leakage

Sprint 3 now also adds the first product-owned note-routing path, which must:

- resolve the default articulation when a trigger omits one
- route note and velocity pairs to the expected zone for the reference instrument
- fail loudly when no articulation or zone mapping exists for a trigger
- hand the resolved zone directly to the existing voice and streaming path

Sprint 3 now also adds the first product-owned load-profile policy, which must:

- resolve named `eco`, `balanced`, and `performance` profiles by stable product-owned ids
- clamp per-voice prefetch to the selected profile budget
- let the shared streaming service downgrade cache budgets without invalidating active leases
- allow dormant resident pages to be purged explicitly after active playback releases them

Sprint 3 now also adds the first product-owned runtime counters, which must:

- count page misses when voices stall at streamed boundaries
- record prefetch-head usage before streamed-page waits begin
- expose read latency statistics from queued background page reads
- surface active and peak voice counts through the streaming runtime
- record purge activity during both automatic eviction and explicit dormant cleanup

The reader still does not:

- schedule asynchronous I/O
- own cache lifetime
- execute playback voices

## Why this matters

Sprint 1 is where the project stops saying "we will eventually have a native runtime model" and starts loading one. The format can still evolve, but it is now a concrete seam with tests, fixtures, and explicit versioning.
