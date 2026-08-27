# Performance Package Content Protection Traceability

Status: Accepted for implementation

Date: 2026-08-27

| Requirement | Implementation seam | Required evidence | Release gate |
| --- | --- | --- | --- |
| Protected runtime settings | V3 encrypted runtime record; package activation | Plaintext scan; wrong-key and mutation tests | P2/P5 |
| Protected UI controls/artwork | V3 encrypted control/artwork records | Known-field and image-signature scan; replacement rejection | P2/P4 |
| No original WAV extraction | Runtime compile and internal sample-page writer | No RIFF/WAVE/path scan; record-carving exercise | P2/P5 |
| Confidential record content | AEAD provider and content-key provider | Upstream vectors; wrong-key/tag/AAD failures | P1/P2 |
| Unauthorized modification rejected | Ed25519 signature and AEAD verification | Header/TOC/record mutation matrix; compromised-symmetric-key test | P3/P5 |
| Publishing origin verified | Trust store and signing-key ID | Wrong/unknown/revoked signing-key tests | P3 |
| Unique nonce per key | Secure RNG and record writer | Repeated-export nonce uniqueness test | P1/P2 |
| Bounded page opening | PackagePagedSampleDataSource and worker | Requested-range and memory metrics | P2/P5 |
| No audio-thread crypto/I/O | Deferred workers and playback context | Real-time guard and sustained playback tests | P4/P5 |
| Legacy compatibility | PackageReaderDispatch V1/V2 paths | Existing corpus and no-rewrite tests | P4 |
| Honest claims | ADR, docs, UI/support messaging | Documentation/security review | P0/P5 |
