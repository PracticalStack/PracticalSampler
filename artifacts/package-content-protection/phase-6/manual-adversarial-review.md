# Performance Package V3 manual adversarial review

Date: 2026-08-28  
Platform: Windows x64  
Result: PASS at the declared extraction-resistance boundary

## Scope

The review used the Release Accurate Salamander V3 package (2,623,997,763 bytes,
40,783 records) and the checked-in negative/mutation fixtures. The review does
not claim DRM, copy prevention, or protection from plaintext captured from a
running authorized process.

## Results

- Archive inspection: `tar -tf` rejected the file as an unrecognized archive.
- Hex inspection: the file begins with the documented `DRSPKG3` V3 framing;
  package identity and public key identifiers are visible by design, while
  settings, artwork, and sample record payloads are AEAD ciphertext.
- Record carving: searches for `RIFF`, `WAVE`, PCM canaries, UI
  canaries, and private-key PEM markers returned no match in the 2.62 GB file.
- Key-string scan: no signing private-key marker or protected release-key bytes
  were present. Public key IDs are non-secret routing metadata.
- Repacking: modifying a TOC identity or payload byte either fails structural
  parsing or Ed25519ph publisher verification. Possession of the symmetric
  content key does not permit generation of a trusted publisher signature.
- Persistent plaintext: automated scans covered the package, export log, host
  state, crash context, staging state, and temp state with zero canary findings.

## Reproduction

The automated equivalents run in `drs.package_v3`,
`drs.package_v3.fuzz_smoke`, and `drs.package_protection.release_gate`. The
machine-readable aggregate is `qualification-evidence.json`; full corpus
measurements are in `../../large-instrument-streaming/phase-7/accurate-salamander-v3-qualification.json`.
