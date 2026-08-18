# Waveform Region Phase 7 REAPER Host Matrix

Result: PASS

Configuration: Release VST3 in REAPER 7.39/win64, isolated configuration per case.

The current host-state fixture was serialized and restored by the Release host-state test before the matrix. This replaced an archived `DecentRhapsodyStudio` binding and obsolete manifest/prepared digests. The refreshed fixture rendered a source and freshly restored processor at the same nonzero peak before REAPER qualification.

| Editor | Sample rate | Block | MIDI notes | Nonzero peak probes | Nonfinite probes | RMS proxy |
|---|---:|---:|---:|---:|---:|---:|
| Closed | 44,100 | 128 | 28 | 1,828 | 0 | 0.149518 |
| Closed | 44,100 | 256 | 28 | 1,776 | 0 | 0.151812 |
| Closed | 44,100 | 512 | 28 | 1,772 | 0 | 0.151638 |
| Closed | 48,000 | 128 | 28 | 1,856 | 0 | 0.149639 |
| Closed | 48,000 | 256 | 28 | 1,848 | 0 | 0.151665 |
| Closed | 48,000 | 512 | 28 | 1,852 | 0 | 0.150610 |
| Open | 44,100 | 128 | 28 | 1,844 | 0 | 0.154414 |
| Open | 44,100 | 256 | 28 | 1,636 | 0 | 0.153227 |
| Open | 44,100 | 512 | 28 | 1,624 | 0 | 0.152255 |
| Open | 48,000 | 128 | 28 | 1,652 | 0 | 0.150829 |
| Open | 48,000 | 256 | 28 | 1,644 | 0 | 0.152449 |
| Open | 48,000 | 512 | 28 | 1,660 | 0 | 0.152768 |

All cases reported the plug-in enabled, online, the intended editor-open state, an inserted 28-note MIDI validation sequence, nonzero finite output, and zero nonfinite peak observations.

