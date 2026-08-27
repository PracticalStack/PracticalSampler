#include <drs/engine/PackageProtectionDiagnostics.h>

namespace drs::engine
{
const char* toString(const PackageProtectionFailure failure) noexcept
{
    switch (failure)
    {
        case PackageProtectionFailure::unsupportedFormat: return "unsupported-format";
        case PackageProtectionFailure::unknownKey: return "unknown-key";
        case PackageProtectionFailure::unavailableKey: return "unavailable-key";
        case PackageProtectionFailure::badSignature: return "bad-signature";
        case PackageProtectionFailure::aeadFailure: return "aead-failure";
        case PackageProtectionFailure::corruption: return "corruption";
        case PackageProtectionFailure::compatibilityMismatch: return "compatibility-mismatch";
        case PackageProtectionFailure::cancellation: return "cancelled";
        case PackageProtectionFailure::ioFailure: return "io-failure";
    }
    return "unknown";
}

std::string redactedPackageProtectionMessage(const PackageProtectionFailure failure)
{
    return std::string("Performance package security failure: ") + toString(failure)
           + ". No protected package content was published.";
}
} // namespace drs::engine
