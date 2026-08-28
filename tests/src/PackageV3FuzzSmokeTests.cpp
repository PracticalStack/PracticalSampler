#include <drs/engine/PackageCrypto.h>
#include <drs/engine/PackageV3.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace
{
std::uint64_t next(std::uint64_t& state)
{
    state ^= state << 13u;
    state ^= state >> 7u;
    state ^= state << 17u;
    return state;
}

std::string field(const std::vector<std::uint8_t>& bytes, const std::size_t offset)
{
    if (offset >= bytes.size()) return {};
    const auto count = std::min<std::size_t>(bytes.size() - offset, 128u);
    return { reinterpret_cast<const char*>(bytes.data() + offset), count };
}
}

int main()
{
    using namespace drs::engine;
    constexpr std::size_t cases = 100'000u;
    std::uint64_t state = 0x8b5ad4cef9172361ull;
    for (std::size_t iteration = 0; iteration < cases; ++iteration)
    {
        const auto size = static_cast<std::size_t>(next(state) % 4097u);
        std::vector<std::uint8_t> bytes(size);
        for (auto& byte : bytes)
            byte = static_cast<std::uint8_t>(next(state));
        auto package = parsePackageV3(bytes);
        static_cast<void>(parsePackageV3Index(bytes, static_cast<std::uint64_t>(bytes.size())));
        if (package.opened)
        {
            std::string issue;
            static_cast<void>(verifyPackageV3Signature(
                bytes, std::vector<PackageSigningKey> {}, package, issue));
            package.signatureVerified = true;
            SecureBuffer releaseKey(std::vector<std::uint8_t>(securePackageKeySizeBytes, 0u));
            SecureBuffer contentKey;
            static_cast<void>(unwrapPackageV3ContentKey(
                package, releaseKey, contentKey, issue));
        }
        const auto quarter = bytes.size() / 4u;
        static_cast<void>(buildPackageV3RecordAad(
            field(bytes, 0u), field(bytes, quarter), static_cast<std::uint32_t>(iteration),
            field(bytes, quarter * 2u), field(bytes, quarter * 3u),
            static_cast<std::uint32_t>(size), static_cast<std::uint32_t>(size >> 1u), size));
    }
    std::cout << "Package V3 deterministic fuzz smoke passed: " << cases << " cases\n";
    return 0;
}
