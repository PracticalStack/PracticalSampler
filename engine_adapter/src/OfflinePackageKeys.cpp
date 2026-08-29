#include <drs/engine/PackageKeys.h>

#include <drs/engine/PackageCrypto.h>
#include <drs/engine/PackageOfflineProtection.generated.h>

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

#include <sodium/utils.h>

namespace drs::engine
{
namespace
{
bool validState(const char* state) noexcept
{
    return state != nullptr
        && (std::string_view(state) == "active"
            || std::string_view(state) == "retired"
            || std::string_view(state) == "revoked");
}

bool hasReservedIdentifierToken(const std::string_view value) noexcept
{
    constexpr std::array<std::string_view, 7> reserved {
        "test", "tests", "dev", "debug", "fixture", "example", "development" };
    std::size_t tokenStart = 0;
    while (tokenStart < value.size())
    {
        while (tokenStart < value.size()
               && (value[tokenStart] == '.' || value[tokenStart] == '_'
                   || value[tokenStart] == '-'))
            ++tokenStart;
        const auto tokenEnd = value.find_first_of("._-", tokenStart);
        const auto token = value.substr(tokenStart,
                                         tokenEnd == std::string_view::npos
                                             ? value.size() - tokenStart
                                             : tokenEnd - tokenStart);
        for (const auto reservedToken : reserved)
        {
            if (token.size() == reservedToken.size())
            {
                bool equal = true;
                for (std::size_t index = 0; index < token.size(); ++index)
                {
                    auto character = token[index];
                    if (character >= 'A' && character <= 'Z')
                        character = static_cast<char>(character - 'A' + 'a');
                    if (character != reservedToken[index])
                    {
                        equal = false;
                        break;
                    }
                }
                if (equal)
                    return true;
            }
        }
        if (tokenEnd == std::string_view::npos)
            break;
        tokenStart = tokenEnd;
    }
    return false;
}

bool hasUsableKeyFragments(const offline_generated::ReleaseKeySlot& slot) noexcept
{
    bool hasNonZeroByte = false;
    for (std::size_t index = 0; index < securePackageKeySizeBytes; ++index)
        hasNonZeroByte = hasNonZeroByte
            || ((slot.mask[index] ^ slot.xorFragment[index]) != 0);
    return hasNonZeroByte;
}

PackageReleaseKeyState toState(const char* state) noexcept
{
    if (std::string_view(state) == "retired") return PackageReleaseKeyState::retired;
    if (std::string_view(state) == "revoked") return PackageReleaseKeyState::revoked;
    return PackageReleaseKeyState::active;
}

bool configured() noexcept
{
    if (offline_generated::profileId == nullptr
        || std::string_view(offline_generated::profileId).empty()
        || hasReservedIdentifierToken(offline_generated::profileId)
        || offline_generated::releaseKeys.empty())
        return false;
    for (std::size_t slotIndex = 0;
         slotIndex < offline_generated::releaseKeys.size();
         ++slotIndex)
    {
        const auto& slot = offline_generated::releaseKeys[slotIndex];
        if (slot.keyId == nullptr || std::string_view(slot.keyId).empty()
            || hasReservedIdentifierToken(slot.keyId)
            || ! validState(slot.state)
            || slot.activatedUtc == nullptr || std::string_view(slot.activatedUtc).empty())
            return false;
        for (std::size_t priorIndex = 0; priorIndex < slotIndex; ++priorIndex)
        {
            const auto& priorSlot = offline_generated::releaseKeys[priorIndex];
            if (priorSlot.keyId != nullptr
                && std::string_view(priorSlot.keyId) == slot.keyId)
                return false;
        }
        if (std::string_view(slot.state) == "retired"
            && (slot.retiredUtc == nullptr || std::string_view(slot.retiredUtc).empty()))
            return false;
        if (std::string_view(slot.state) == "revoked"
            && (slot.revokedUtc == nullptr || std::string_view(slot.revokedUtc).empty()))
            return false;
        if (std::string_view(slot.state) == "active"
            && (slot.retiredUtc != nullptr || slot.revokedUtc != nullptr))
            return false;
        if (std::string_view(slot.state) == "retired" && slot.revokedUtc != nullptr)
            return false;
        if (std::string_view(slot.state) == "revoked" && slot.retiredUtc != nullptr)
            return false;
        if (! hasUsableKeyFragments(slot))
            return false;
    }
    return true;
}
} // namespace

bool OfflinePackageReleaseKeySource::isConfigured() const noexcept
{
    return configured();
}

bool OfflinePackageReleaseKeySource::loadReleaseKey(
    const std::string& packageId,
    const std::string& keyId,
    SecureBuffer& key) const
{
    key.clear();
    if (! configured() || packageId.empty() || keyId.empty())
        return false;
    const auto found = std::find_if(offline_generated::releaseKeys.begin(),
                                    offline_generated::releaseKeys.end(),
        [&](const auto& slot) { return keyId == slot.keyId; });
    if (found == offline_generated::releaseKeys.end()
        || std::string_view(found->state) == "revoked")
        return false;

    std::array<std::uint8_t, securePackageKeySizeBytes> reconstructed {};
    bool hasNonZeroByte = false;
    for (std::size_t index = 0; index < reconstructed.size(); ++index)
    {
        reconstructed[index] = found->mask[index] ^ found->xorFragment[index];
        hasNonZeroByte = hasNonZeroByte || reconstructed[index] != 0;
    }
    if (! hasNonZeroByte)
    {
        sodium_memzero(reconstructed.data(), reconstructed.size());
        return false;
    }

    key = SecureBuffer(std::vector<std::uint8_t>(reconstructed.begin(), reconstructed.end()));
    sodium_memzero(reconstructed.data(), reconstructed.size());
    return key.size() == securePackageKeySizeBytes;
}

const char* offlinePackageProtectionProfileId() noexcept
{
    return offline_generated::profileId;
}

const char* offlinePackageReleaseKeyId() noexcept
{
    for (const auto& slot : offline_generated::releaseKeys)
    {
        if (slot.state != nullptr && std::string_view(slot.state) == "active")
            return slot.keyId == nullptr ? "" : slot.keyId;
    }
    return "";
}

const std::vector<PackageReleaseKeyPolicy>& offlinePackageReleaseKeyPolicies() noexcept
{
    static const auto policies = []
    {
        std::vector<PackageReleaseKeyPolicy> result;
        if (! configured()) return result;
        result.reserve(offline_generated::releaseKeys.size());
        for (const auto& slot : offline_generated::releaseKeys)
        {
            PackageReleaseKeyPolicy policy;
            policy.keyId = slot.keyId;
            policy.state = toState(slot.state);
            policy.activatedUtc = slot.activatedUtc;
            policy.retiredUtc = slot.retiredUtc == nullptr ? "" : slot.retiredUtc;
            policy.revokedUtc = slot.revokedUtc == nullptr ? "" : slot.revokedUtc;
            result.push_back(std::move(policy));
        }
        return result;
    }();
    return policies;
}

bool offlinePackageProtectionProfileConfigured() noexcept
{
    return configured();
}
} // namespace drs::engine
