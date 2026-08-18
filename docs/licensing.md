# Practical Sampler V1 Licensing Statement

Effective August 18, 2026.

## Project-owned software

Unless a file or directory states otherwise, original Practical Sampler source code, build scripts,
tests, documentation, and original user-interface assets are licensed under the **GNU Affero General
Public License, version 3 only** (`AGPL-3.0-only`). The complete license text is in
[`LICENSE`](../LICENSE).

Copyright (C) 2026 Practical Sampler contributors.

Practical Sampler is free software: you may use, study, modify, and redistribute it under the terms
of the AGPLv3. Practical Sampler is provided without warranty; see the license for the complete terms
and conditions.

## Why AGPLv3

The V1 open-source build uses the open-source licensing paths of its principal frameworks:

- The vendored JUCE modules are dual-licensed under AGPLv3 or a commercial JUCE license; see
  [`third_party/juce/LICENSE.md`](../third_party/juce/LICENSE.md).
- The vendored HISE snapshot is offered under GPLv3 or a commercial HISE license; see the
  [HISE license statement](../third_party/hise/README.md#license).
- Section 13 of AGPLv3 expressly permits AGPLv3-covered work to be linked or combined with GPLv3
  work. Each part retains its applicable license, and the AGPLv3 terms continue to apply to the
  AGPL-covered part.

Accordingly, `AGPL-3.0-only` is the V1 project license selected for Practical Sampler's original
code. This repository does not grant a commercial JUCE license, a commercial HISE license, or a
proprietary license to Practical Sampler code.

## Third-party software

Code under `third_party/` and any other material identified as third-party remains under its own
copyright and license terms. Practical Sampler's AGPL license does not replace or relicense those
terms. Consult [`third_party/vendor-manifest.md`](../third_party/vendor-manifest.md), the license
files shipped beside each dependency, and source-level notices before redistribution.

## Samples, instruments, and other content

This project license does not grant rights to sample audio, imported libraries, third-party artwork,
fonts, demo instruments, reference corpora, or playable packages that carry separate terms or are
not owned by the Practical Sampler contributors. Do not assume that unmarked content is reusable;
verify its provenance and license first.

Instrument authors remain responsible for ensuring that they have permission to use and distribute
their samples and other content. A project or `.drpkg` package may include its own `LICENSE.txt`.
Executable products that incorporate Practical Sampler, JUCE, or HISE code remain subject to the
applicable software licenses regardless of the content license.

## Distribution note

Anyone distributing modified source or binaries must comply with the complete AGPLv3 terms and all
applicable third-party notices. Among other requirements, distributors should preserve copyright and
license notices, provide the complete corresponding source in an allowed manner, and retain the
required legal notices in interactive interfaces. Read the full license rather than relying on this
summary.

This document records the project's V1 licensing policy; it is not legal advice.
