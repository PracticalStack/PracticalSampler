# Performance package protection operations

## Release and migration

New exports use `DRSPKG3`, XChaCha20-Poly1305 records, a fresh package content
key, and the release-key/signature workflow. V1 and V2 remain read-only legacy
compatibility formats. Existing packages are migrated only by opening the
authoring project and explicitly re-exporting; opening an old file never
rewrites or silently upgrades it.

Production signing is performed by Practical Sampler release infrastructure.
Desktop authoring builds do not contain a production signing private key. The
reader ships only the pinned public trust store and obtains release/content
keys through the `PackageKeyProvider` boundary.

## Support diagnostics

Support tools may report only the stable failure category (`unsupported-format`,
`unknown-key`, `unavailable-key`, `bad-signature`, `aead-failure`, `corruption`,
`compatibility-mismatch`, `cancelled`, or `io-failure`). Logs must not contain
package paths, plaintext, keys, nonces, tags, or encrypted payload bytes.

## Incident handling

Rotate release keys through the controlled pipeline, retain only the reader
keys required by the compatibility policy, and publish an emergency reader
update when compromise or revocation requires it. A compromised symmetric
content key does not authorize modified packages because publisher signature
verification is independent of AEAD decryption.
