#pragma once

#include <string>

#include <drs/engine/SecureBuffer.h>

namespace drs::engine
{
// Runtime key lookup boundary. Implementations may resolve through an
// entitlement service, an OS-protected store, or a test fixture; callers never
// derive keys from package metadata and never need to know the backing store.
class PackageKeyProvider
{
public:
    virtual ~PackageKeyProvider() = default;
    virtual bool resolvePackageKey(const std::string& packageId,
                                   const std::string& keyId,
                                   SecureBuffer& key,
                                   std::string& issue) const = 0;
};
} // namespace drs::engine
