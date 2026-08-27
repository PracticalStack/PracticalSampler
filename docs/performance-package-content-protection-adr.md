# ADR: Performance Package Content Protection

Status: Accepted for implementation

Date: 2026-08-27

Owners: Practical Sampler runtime, release engineering, and security review

## Context

`PackageCrypto.cpp` currently identifies a custom deterministic transform as
`drs.sha256.stream-seal.v1`. The implementation uses a source-visible seed,
FNV-style hashing, a custom mixer, XOR transformation, deterministic nonces,
and a forgeable custom tag. It is useful for fixture round trips and casual
corruption detection, but it must not be represented as production
authenticated encryption.

Playable packages are intended to be portable and copyable. The protection
goal is to keep runtime settings, artwork, UI definitions, and sample data
from direct inspection or ordinary extraction, and to reject unauthorized
modification. Customer entitlement, machine binding, revocation, and
unbreakable DRM are explicitly out of scope.

## Decision

1. New packages use a V3 container with XChaCha20-Poly1305 AEAD for every
   protected record. The nonce is 24 cryptographically random bytes, the tag
   is 16 bytes, and each record uses a fresh random package content key.
2. Associated data binds the package and record identity, format version,
   record kind, source generation, page index, plaintext size, and any other
   compatibility identity required by the reader.
3. The package content key is wrapped under a versioned release key. The
   plaintext content key is never serialized and is held only for the export
   or active package generation lifetime.
4. The complete package envelope is signed with Ed25519. The application
   contains public verification keys only. Initial production packages are
   signed by Practical Sampler release infrastructure; desktop authoring
   exports are development/unsigned artifacts unless they pass through that
   controlled publishing path. A hosted author-publishing service may be
   added later without changing the V3 reader contract.
5. V1 and V2 readers remain available under an explicit legacy policy. New
   exports are V3 only. Existing packages are not secured in place; they must
   be re-exported.
6. Decryption, signature verification, file I/O, and page authentication stay
   off the audio callback. No plaintext protected asset is written to a
   temporary file.

## Security promise

Playable packages are encrypted and authenticated at rest and signed to
verify publishing origin. Practical Sampler rejects package mutation,
record substitution, and unofficial repacking. The design prevents direct
inspection of compiled settings and ordinary WAV extraction while not
claiming resistance to a determined attacker who can instrument or patch a
running authorized application, capture process memory, or record its audio.

## Consequences

- Secure nonces make encrypted package bytes intentionally nondeterministic;
  release gates must compare semantic plaintext/digests instead of raw bytes.
- A V3 format and key-envelope identity are required; V1/V2 cannot be made
  secure retroactively.
- A production package publisher must be selected before release. A local
  desktop binary must not contain the publisher private key.
- A shared offline release key remains extractable from a determined client;
  per-package keys, wrapping, signatures, and rotation limit the impact and
  keep the public promise honest.
- The existing V2 bounded record/page architecture is retained so package
  performance and memory behavior remain comparable.

## Alternatives rejected

- Custom hashing/XOR construction: not cryptographically secure.
- Deterministic nonce derivation: unsafe for a reused key and leaks equality.
- Encryption without a signature: a holder of the symmetric key can forge a
  package that passes AEAD verification.
- Embedding a publisher private key in the application: anyone with the
  binary can mint publisher-valid packages.
- Calling this licensing or ownership enforcement: no entitlement or machine
  validation exists in this initiative.
