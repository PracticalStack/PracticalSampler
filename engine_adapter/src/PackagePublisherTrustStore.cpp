#include <drs/engine/PackagePublisherTrustStore.h>

#include <drs/engine/PackageTrustStore.generated.h>

#include <algorithm>
#include <set>
#include <utility>

namespace drs::engine
{
namespace
{
int hexNibble(const char value) noexcept
{
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}

std::vector<std::uint8_t> decodeHex(const std::string& hex)
{
    if (hex.size() % 2u != 0u) return {};
    std::vector<std::uint8_t> bytes;
    bytes.reserve(hex.size() / 2u);
    for (std::size_t offset = 0; offset < hex.size(); offset += 2u)
    {
        const auto high = hexNibble(hex[offset]);
        const auto low = hexNibble(hex[offset + 1u]);
        if (high < 0 || low < 0) return {};
        bytes.push_back(static_cast<std::uint8_t>((high << 4) | low));
    }
    return bytes;
}

std::vector<std::string> split(const std::string& value, const char delimiter)
{
    std::vector<std::string> fields;
    std::size_t begin = 0;
    while (begin <= value.size())
    {
        const auto end = value.find(delimiter, begin);
        fields.push_back(value.substr(begin, end == std::string::npos
            ? std::string::npos : end - begin));
        if (end == std::string::npos) break;
        begin = end + 1u;
    }
    return fields;
}

bool parseState(const std::string& value, PackageSigningKeyState& state)
{
    if (value == "active") { state = PackageSigningKeyState::active; return true; }
    if (value == "retired") { state = PackageSigningKeyState::retired; return true; }
    if (value == "revoked") { state = PackageSigningKeyState::revoked; return true; }
    return false;
}
} // namespace

PackagePublisherTrustStore::PackagePublisherTrustStore(
    std::vector<PackageSigningKey> keys)
    : keys_(std::move(keys))
{
    std::set<std::string> keyIds;
    for (const auto& key : keys_)
    {
        if (key.keyId.empty() || ! keyIds.insert(key.keyId).second)
        {
            configurationIssue_ = "package-recognition store contains an empty or duplicate key id";
            return;
        }
        if (key.publicKey.size() != packageEd25519PublicKeyBytes
            || key.activatedUtc.empty())
        {
            configurationIssue_ = "package-recognition store key material or activation metadata is invalid";
            return;
        }
        if (key.state == PackageSigningKeyState::retired && key.retiredUtc.empty())
        {
            configurationIssue_ = "retired package-recognition key is missing retirement metadata";
            return;
        }
        if (key.state == PackageSigningKeyState::revoked && key.revokedUtc.empty())
        {
            configurationIssue_ = "revoked package-recognition key is missing revocation metadata";
            return;
        }
    }
    valid_ = true;
}

bool PackagePublisherTrustStore::resolvePublicKey(
    const std::string& keyId,
    std::vector<std::uint8_t>& publicKey,
    std::string& issue) const
{
    publicKey.clear();
    if (! valid_)
    {
        issue = "package-recognition store configuration is invalid";
        return false;
    }
    const auto found = std::find_if(keys_.begin(), keys_.end(),
        [&](const auto& key) { return key.keyId == keyId; });
    if (found == keys_.end())
    {
        issue = "package signing key id is unknown";
        return false;
    }
    if (found->state == PackageSigningKeyState::revoked)
    {
        issue = "package signing key is revoked";
        return false;
    }
    publicKey = found->publicKey;
    issue.clear();
    return true;
}

const PackagePublisherTrustStore& builtInPackagePublisherTrustStore()
{
    static const PackagePublisherTrustStore store([]
    {
        std::vector<PackageSigningKey> keys;
        const std::string entries = DRS_PACKAGE_PUBLISHER_TRUST_ENTRIES;
        if (! entries.empty())
        {
            for (const auto& encoded : split(entries, ';'))
            {
                const auto fields = split(encoded, '|');
                PackageSigningKey key;
                if (fields.size() == 6u && parseState(fields[2], key.state))
                {
                    key.keyId = fields[0];
                    key.publicKey = decodeHex(fields[1]);
                    key.activatedUtc = fields[3];
                    key.retiredUtc = fields[4];
                    key.revokedUtc = fields[5];
                }
                keys.push_back(std::move(key));
            }
            return keys;
        }
        const std::string keyId = DRS_PACKAGE_PUBLISHER_KEY_ID;
        const std::string publicKeyHex = DRS_PACKAGE_PUBLISHER_PUBLIC_KEY_HEX;
        if (! keyId.empty() || ! publicKeyHex.empty())
        {
            PackageSigningKey key;
            key.keyId = keyId;
            key.publicKey = decodeHex(publicKeyHex);
            key.state = PackageSigningKeyState::active;
            key.activatedUtc = DRS_PACKAGE_PUBLISHER_KEY_ACTIVATED_UTC;
            keys.push_back(std::move(key));
        }
        return keys;
    }());
    return store;
}
} // namespace drs::engine
