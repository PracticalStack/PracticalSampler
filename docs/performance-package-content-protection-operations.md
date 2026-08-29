# Performance package protection operations

## Release and migration

New exports use `DRSPKG3`, XChaCha20-Poly1305 records, a fresh package content
key, and the release-key/signature workflow. V1 and V2 remain read-only legacy
compatibility formats. Existing packages are migrated only by opening the
authoring project and explicitly re-exporting; opening an old file never
rewrites or silently upgrades it.

Legacy creator migration procedure:

1. Locate the original editable `.drsproj` and its source content. A V1/V2
   `.drpkg` is a read-only playback input and is not an authoring source.
2. Open the project in a current, production-provisioned Practical Sampler
   authoring build and resolve every missing sample or artwork reference.
3. Choose **Export Playable Instrument**. A successful export must report V3,
   controlled publisher signing, staged verification, and atomic publication.
4. Reopen the new package through the same plugin/standalone activation path
   used by customers before distributing it.
5. Retain the legacy package only as an explicitly unprotected compatibility
   artifact. Do not describe it as encrypted, authenticated, or protected.

If the original authoring project is unavailable, there is no supported
in-place conversion or extraction workflow. Support may keep the legacy file
for read-only playback, but must not rewrite it, wrap it, or represent it as V3.

Production signing is performed by Practical Sampler release infrastructure.
Desktop authoring builds do not contain a production signing private key. The
reader ships only the pinned public trust store and obtains release/content
keys through the `PackageKeyProvider` boundary.

Production application targets do not link the V1/V2 writer implementation.
Those writers and the deterministic compatibility crypto provider are confined
to an explicit test-fixture library; V1/V2 readers remain available only for
bounded, read-only compatibility.

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
