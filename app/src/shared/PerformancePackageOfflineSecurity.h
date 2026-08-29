#pragma once

#include "shared/PerformancePackageExportService.h"

#include "drs/engine/PackageReaderDispatch.h"

#include <memory>

namespace drs::app
{
// Builds the release-profile security context used by production export. The
// factory returns null when the release profile is absent, malformed, or has
// no matching public recognition key; development and test builds therefore
// remain fail-closed until explicitly configured.
std::shared_ptr<const PerformancePackageExportSecurityContext>
makeOfflinePerformancePackageExportSecurityContext();

// Builds the read-only half of the same profile for portable package loading.
// Loading does not require the private signing key, but it does require a
// configured release key policy and a public recognition key store.
std::shared_ptr<const drs::engine::PerformancePackageV3ActivationSecurityContext>
makeOfflinePerformancePackageActivationSecurityContext();
} // namespace drs::app
