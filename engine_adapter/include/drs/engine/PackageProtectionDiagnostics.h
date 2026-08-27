#pragma once

#include <string>

namespace drs::engine
{
enum class PackageProtectionFailure
{
    unsupportedFormat,
    unknownKey,
    unavailableKey,
    badSignature,
    aeadFailure,
    corruption,
    compatibilityMismatch,
    cancellation,
    ioFailure
};

const char* toString(PackageProtectionFailure failure) noexcept;
// Returns a stable support-facing message. Inputs such as paths, key bytes,
// nonces, tags, and attacker-controlled package strings are never included.
std::string redactedPackageProtectionMessage(PackageProtectionFailure failure);
} // namespace drs::engine
