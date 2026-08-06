# Package v2 corruption and bounds matrix

Result: PASS

Configuration: Release x64, MSVC 19.44, Windows 11.

Command:

`ctest --test-dir build/vs2022-release -C Release --output-on-failure -R ^drs\.package_v2\.records$`

The `drs.package_v2.records` test passed. Its production-reader matrix verifies:

- metadata/TOC open without record plaintext materialization;
- exact single-record range read, authentication, and checksum;
- cancellation before record I/O with zero bytes read;
- independent checksum sabotage and ciphertext authentication failure;
- locality: a corrupt page does not invalidate an unrelated page;
- rejection of a 64 KiB + 1 plaintext record and duplicate record identity;
- checked truncation and 64-bit record-region bounds;
- a sparse record at 5 GiB and a page identity of 7,000,000,000;
- package-backed head/page publication only after worker verification;
- canceled/corrupt pages remain unpublished while prior ready data remains usable;
- small v1 deterministic compatibility without rewrite-on-load;
- oversized v1 rejection before either resident reader/inspection path, with v2 re-export guidance;
- staged streaming export, sampled structural verification, atomic publication, and cleanup after cancellation;
- a sparse 1 GiB cancellation plan bounded to at most 64 KiB plaintext and less than 65 KiB sealed working buffers.

The same test passed in the final Debug focused matrix. No corruption, authentication, lifetime, or unbounded-record finding remains.
