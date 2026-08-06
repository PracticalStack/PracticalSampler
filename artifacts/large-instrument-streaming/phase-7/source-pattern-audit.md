# Production source-pattern audit

Result: PASS

Scope: `app/src`, `engine_adapter/src`, and `engine_adapter/include` after the final Release build.

Search families included whole-file/entire-file helpers, `loadFileAsData`, `MemoryBlock`, byte-vector payloads, decoded PCM ownership, synthetic/simulated page services, payload copies, and the v1/v2/background-image size ceilings.

## Findings and disposition

| Finding | Disposition |
|---|---|
| Package v2 metadata and audio | Production v2 opens header/TOC/metadata with bounded ranges and opens at most one independently authenticated record. Plaintext records are capped at 64 KiB. No total-audio-sized v2 buffer remains. |
| Streaming export | Production export builds lazy head/page record loaders and keeps one plaintext/sealed record at a time. Real-corpus peaks were 65,536/65,592 bytes for a 2.63 GB package. |
| Package v1 byte vectors and whole-file verification | Intentional resident compatibility path only. Both dispatch and direct inspection reject files above `maximumResidentV1PackageBytes` (64 MiB) before whole-file read. Larger v1 packages require v2 re-export. |
| `PreparedPlaybackDecodedSampleData` | Intentional D-01 small-resident path behind the common sample-data-source contract. The 512 MiB admission check occurs before PCM realization; the real 2.62 GB estimate selected streaming and decoded zero bytes. |
| Artwork `MemoryBlock`/byte vector | Intentional package artwork path with a checked 16 MiB limit before `loadFileAsData`, in both export and processor paths. It cannot scale with audio corpus size. |
| Metadata byte vectors | Manifest, instrument, and stream-index JSON are bounded metadata records and are chunked to the 64 KiB record policy; they do not contain sample PCM. |
| Legacy `RuntimeStreamingService` synthetic page generator | Retained for the older reference-instrument/runtime-voice simulator and its tests. Package sessions bypass it: `EngineFacade` returns real prepared-worker counters when a package activation payload exists. Authoring and package sampler activation use `SampleDataSource`, the real WAV/package providers, and callback page intents. It is not a bridge in the qualified large-instrument path. |
| Unused whole-payload reader helpers | Removed from `PackageReader.cpp`; no byte-buffer package-open bridge remains there. |

Production readiness is derived from admitted descriptors, prepared heads, a built immutable render model, staged activation, and callback cutover. Metadata acceptance alone is never labeled playable.
