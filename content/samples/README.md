# Native Sample Content

This directory is the product-owned native source-sample root for Practical Sampler.

Phase 2 populates this root with the reusable product fixtures `DRS_Sine_A3.wav` and
`DRS_TriangleLead_A4.wav`. New source audio should land here only when it is product-owned or its
license and provenance are recorded.

| File | Stable ID | Format | Provenance |
| --- | --- | --- | --- |
| `DRS_Sine_A3.wav` | `sine-a3` | WAV | Retained development fixture from the pre-native Practical Sampler sample set. |
| `DRS_TriangleLead_A4.wav` | `triangle-a4` | WAV | Retained development fixture from the pre-native Practical Sampler sample set. |

## Contract

- Files are source audio owned or explicitly licensed for Practical Sampler development.
- Paths referenced by native `.drsproj` and `.drinst` fixtures are relative to this directory or to
  the product's declared project content root; they must not reference legacy authoring-tool layouts.
- The native runtime consumes audio files directly or through product-owned package formats. Legacy
  sample maps and authoring conventions are not part of this contract.
- Any new fixture should document its stable identifier, source path, format, and provenance in the
  owning runtime fixture or a future native sample manifest.
