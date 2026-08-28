#include <drs/engine/PackageCrypto.h>
#include <drs/engine/PackageV3.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#ifndef DRS_PACKAGE_V3_FUZZ_MODE
#define DRS_PACKAGE_V3_FUZZ_MODE 0
#endif

namespace
{
constexpr std::size_t maximumFuzzInputBytes = 1024u * 1024u;

std::string boundedString(const std::uint8_t* data,
                          const std::size_t size,
                          const std::size_t offset)
{
    if (offset >= size) return {};
    const auto count = std::min<std::size_t>(size - offset, 128u);
    return { reinterpret_cast<const char*>(data + offset), count };
}
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    if (data == nullptr || size == 0u || size > maximumFuzzInputBytes)
        return 0;
    std::vector<std::uint8_t> bytes(data, data + size);
    using namespace drs::engine;
#if DRS_PACKAGE_V3_FUZZ_MODE == 0
    static_cast<void>(parsePackageV3(bytes));
#elif DRS_PACKAGE_V3_FUZZ_MODE == 1
    static_cast<void>(parsePackageV3Index(bytes, static_cast<std::uint64_t>(size)));
#elif DRS_PACKAGE_V3_FUZZ_MODE == 2
    auto package = parsePackageV3(bytes);
    if (package.opened)
    {
        std::string issue;
        static_cast<void>(verifyPackageV3Signature(bytes, std::vector<PackageSigningKey> {},
                                                   package, issue));
    }
#elif DRS_PACKAGE_V3_FUZZ_MODE == 3
    auto package = parsePackageV3(bytes);
    if (package.opened)
    {
        package.signatureVerified = true;
        SecureBuffer releaseKey(std::vector<std::uint8_t>(securePackageKeySizeBytes, 0u));
        SecureBuffer contentKey;
        std::string issue;
        static_cast<void>(unwrapPackageV3ContentKey(package, releaseKey, contentKey, issue));
    }
#else
    const auto quarter = size / 4u;
    static_cast<void>(buildPackageV3RecordAad(
        boundedString(data, size, 0u), boundedString(data, size, quarter),
        static_cast<std::uint32_t>(size), boundedString(data, size, quarter * 2u),
        boundedString(data, size, quarter * 3u),
        static_cast<std::uint32_t>(size >> 1u),
        static_cast<std::uint32_t>(size >> 2u),
        static_cast<std::uint64_t>(size)));
#endif
    return 0;
}
