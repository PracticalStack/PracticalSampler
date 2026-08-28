#pragma once

#include <drs/engine/PackagePublisherSigning.h>
#include <drs/engine/PackagePublisherTrustStore.h>
#include <drs/engine/PackageSignature.h>

#include <string>
#include <fstream>
#include <utility>
#include <vector>

class PackageProtectionTestSigner final
    : public drs::engine::PackagePublisherSigningClient
{
public:
    PackageProtectionTestSigner(std::string keyId,
                                const std::vector<std::uint8_t>& privateKey)
        : keyId_(std::move(keyId)), privateKey_(privateKey)
    {
    }

    bool signCanonicalPackage(
        const drs::engine::PackagePublisherSigningRequest& request,
        drs::engine::PackagePublisherSigningResponse& response,
        std::string& issue) const override
    {
        response = {};
        std::vector<std::uint8_t> fileBytes;
        const std::vector<std::uint8_t>* signedBytes = request.canonicalSignedBytes;
        if (signedBytes == nullptr && ! request.canonicalSignedFilePath.empty())
        {
            std::ifstream input(request.canonicalSignedFilePath, std::ios::binary);
            fileBytes.assign(std::istreambuf_iterator<char>(input),
                             std::istreambuf_iterator<char>());
            if (input.bad() || fileBytes.size() != request.canonicalSignedBytesLength)
            {
                issue = "test signer could not read the canonical signed file";
                return false;
            }
            signedBytes = &fileBytes;
        }
        if (request.signingKeyId != keyId_ || signedBytes == nullptr
            || ! drs::engine::packageSignEd25519ph(
                privateKey_, *signedBytes, response.signature, issue))
            return false;
        response.auditId = "test-signing:" + keyId_;
        return true;
    }

private:
    std::string keyId_;
    std::vector<std::uint8_t> privateKey_;
};

inline drs::engine::PackageSigningKey activeTestSigningKey(
    const std::string& keyId,
    const std::vector<std::uint8_t>& publicKey)
{
    return { keyId, publicKey, drs::engine::PackageSigningKeyState::active,
             "2026-08-28T00:00:00Z", {}, {} };
}
