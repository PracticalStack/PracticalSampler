# Package crypto dependency record

- Dependency: libsodium 1.0.20-RELEASE.
- Source: `https://github.com/jedisct1/libsodium`, pinned tarball SHA-256
  `8E5AECA07A723A27BBECC3BEEF14B0068D37E7FC0E97F51B3F1C82D2A58005C1`.
- License: ISC; retain the upstream license and notice with redistributable
  builds.
- Used primitives: XChaCha20-Poly1305-IETF, Ed25519, operating-system random
  bytes. No custom cipher or tag implementation is used by V3.
- Update owner: Practical Sampler build/release engineering. Updates require
  provenance/hash review, supported-platform builds, upstream security-advisory
  review, and the focused crypto test suite.
