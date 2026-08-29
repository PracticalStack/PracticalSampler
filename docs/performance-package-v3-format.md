# Performance Package V3 Format Contract

Status: Implemented canonical contract, bounded reader, and CPCA-3 key/signing interfaces

Date: 2026-08-27

## Container identity

- Magic: `DRSPKG3`
- Format version: `3`
- Crypto suite: `xchacha20-poly1305-ietf`
- Signature suite: `ed25519ph` (RFC 8032 prehash variant)
- Key identifiers: opaque `encryptionKeyId` and `signingKeyId`
- Package identity: stable `packageId`
- V1/V2 compatibility is selected by magic/format dispatch, never by
  silently trying multiple algorithms after authentication failure.

## Fixed header

All integers are unsigned little-endian. Offsets are absolute file offsets.
The fixed header is 76 bytes:

| Offset | Size | Field | Required value |
|---:|---:|---|---|
| 0 | 8 | magic | `DRSPKG3\0` |
| 8 | 4 | formatVersion | `3` |
| 12 | 4 | headerSize | fixed plus variable envelope |
| 16 | 4 | flags | `0x00000007` (encrypted, authenticated, signed) |
| 20 | 2 | cryptoSuite | `1` = XChaCha20-Poly1305-IETF |
| 22 | 2 | signatureSuite | `1` = Ed25519ph |
| 24 | 4 | recordCount | `1..131072` |
| 28 | 8 | tocOffset | exactly `headerSize` |
| 36 | 8 | tocSize | exact serialized TOC size |
| 44 | 8 | payloadOffset | exactly `tocOffset + tocSize` |
| 52 | 8 | payloadSize | sum of ciphertext sizes |
| 60 | 8 | signatureOffset | exactly `payloadOffset + payloadSize` |
| 68 | 4 | signatureSize | exactly `64` |
| 72 | 4 | reserved | zero |

The fixed header is followed by four length-prefixed UTF-8 byte strings in
this order: `packageId`, `compatibilityId`, `encryptionKeyId`, and
`signingKeyId`. Each string uses a two-byte length, must be non-empty, and is
limited to 4096 bytes.

The variable header ends with the key envelope:

```text
wrappedKeyNonce[24]
wrappedKeyCiphertextSize:u16 = 32
wrappedKeyCiphertext[32]
wrappedKeyTag[16]
```

`headerSize` must equal the byte immediately following the envelope. Padding
and unrecognized header extensions are rejected in V3.

Reader ceilings are 64 KiB for the complete header, 256 MiB for the complete
TOC, 64 MiB for one record, 131,072 records, and 16 GiB for one package. The
fixed header and these section sizes are validated with checked arithmetic
before any variable-size read or allocation.

## Cleartext envelope

The cleartext envelope contains only data required for safe dispatch and
bounds checking:

```text
magic, formatVersion
cryptoSuite, signatureSuite
packageId, encryptionKeyId, signingKeyId
wrappedPackageContentKey
bounded TOC offsets and encrypted sizes
publisher signature
```

Display name, authoring metadata, UI controls, artwork, sample identities,
source paths, runtime settings, and sample content are encrypted records.

## TOC record framing

Records are ordered canonically by `(recordKind, recordId, generation,
pageIndex)`. Duplicate identities and non-canonical ordering are rejected.
Each TOC entry is:

```text
ordinal:u32
recordIdLength:u16 + recordId
recordKindLength:u16 + recordKind
generation:u32
pageIndex:u32
plaintextSize:u64
ciphertextOffset:u64
ciphertextSize:u64 = plaintextSize
nonce[24]
tag[16]
```

Ciphertexts are stored contiguously in canonical TOC order between
`payloadOffset` and `signatureOffset`. Every offset must equal the end of the
previous region; gaps, overlap, padding, trailing bytes, and partial coverage
are rejected.

## Associated data

Canonical AAD is a length-prefixed binary serialization, never delimiter-
joined text:

```text
"DRSAAD3\0"[8]
formatVersion:u32 = 3
cryptoSuite:u16 = 1
ordinal:u32
packageIdLength:u16 + packageId
compatibilityIdLength:u16 + compatibilityId
recordIdLength:u16 + recordId
recordKindLength:u16 + recordKind
generation:u32
pageIndex:u32
plaintextSize:u64
```

Changing any authenticated identity must make record opening fail. The AAD
construction is versioned and tested as a byte-exact contract.

## Signature coverage

The 64-byte Ed25519ph signature is the final file region and covers every byte
from offset zero through `signatureOffset - 1`. This includes the fixed and
variable header, wrapped content key, key identifiers, complete TOC, record
order, every identity, nonce, ciphertext, and authentication tag. No mutable
V3 package byte is outside signature coverage.

Structural parsing does not establish package recognition. Signature
verification against the application recognition store must succeed before
key unwrap or record opening. In the portable offline profile, recognition
means that the package matches the Practical Sampler format and protection
profile; it does not identify an author, establish ownership, or prove
issuance by a controlled publisher.

The file-backed reader reads the fixed 76-byte header first, then only the
bounded header/TOC index. It streams the complete signed region through the
Ed25519ph verifier with a 64 KiB payload buffer. It does not retain resident
ciphertext. After verification, a requested record is read by its verified
offset and independently authenticated/decrypted. These are blocking worker-
side APIs and are forbidden on the audio callback.

## Key envelope

The package content key is a random 32-byte data-encryption key. The active
release-key provider wraps and unwraps it under a key identified by
`encryptionKeyId`. Wrapped-key decoding is bounded and fails closed on an
unknown, retired, or unavailable key.

The signing key is independent from the release encryption key. A signing-key
rotation changes `signingKeyId` and does not require re-encrypting records.

Release and content keys are held in move-only `SecureBuffer` instances and
cleared with `sodium_memzero`. Runtime release-key lookup is exact by versioned
`encryptionKeyId`: active keys may encrypt/decrypt, retired keys decrypt only,
and revoked or unknown keys fail closed. The offline profile may reconstruct
its application protection capability locally; a future licensed profile may
resolve it through an entitlement or OS-protected source.

Package-recognition verification keys are public-only immutable trust-store
entries with activation, retirement, and revocation metadata. Active and
retired public keys recognize packages; revoked and unknown keys fail closed.
V3 writing invokes a `PackagePublisherSigningClient` for wire/API compatibility.
The planned portable offline profile will provide a local implementation; its
signature will not establish author identity or publisher origin. The
controlled signer remains available as a future licensed-publishing profile.

Key-envelope AAD is also length-prefixed binary:

```text
"DRSKEYE3"[8]
packageIdLength:u16 + packageId
encryptionKeyIdLength:u16 + encryptionKeyId
```

## Semantic digest

The semantic digest is SHA-256 over `DRSSEM3\0`, package identity,
compatibility identity, canonical record count, and each canonical record's
ordinal, identity, generation, page index, plaintext length, and plaintext.
Release-key IDs, signing-key IDs, nonces, ciphertext, tags, and signatures are
excluded so equivalent exports retain the same semantic digest while their
encrypted bytes remain intentionally nondeterministic.

## Production streaming export

`PerformancePackageExportService` maps the sanitized runtime manifest,
runtime instrument/UI contract, stream index, background image, license text,
sample heads, and sample pages into canonical V3 records. It does not call a
V1/V2 writer or the deterministic compatibility crypto provider.

The V3 writer loads, hashes, seals, and writes one bounded record at a time.
Only non-secret record descriptors, nonces, and tags remain resident while the
payload is staged. It then finalizes the canonical header/TOC, asks the
publisher client to sign the staged file-backed signed region, appends the
detached signature, verifies the complete signature with the immutable trust
store, opens selected records with AEAD, and atomically replaces the output.
Cancellation or any key, signing, verification, or I/O failure removes the
staging file and preserves the last published package.

The export service requires an injected
`PerformancePackageExportSecurityContext`. An absent or invalid context fails
closed before compilation and cannot select a legacy writer. The portable
offline profile will supply this context from the application-contained key
and recognition implementations; test signers and test keys are not accepted
by the release profile. A future licensed profile may inject managed provider,
signer, and trust objects as described in the key-operations runbook.

## Compatibility policy

- V1/V2 packages remain readable only through their existing bounded legacy
  paths and are never rewritten on open.
- V3 readers reject V4 or unknown future formats with an explicit format
  failure.
- V3 writers never emit the deterministic legacy algorithm.
- Re-export is the only migration path from V1/V2 to V3.
