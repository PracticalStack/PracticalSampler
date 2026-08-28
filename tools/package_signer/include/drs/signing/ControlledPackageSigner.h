#pragma once

#include <drs/engine/PackagePublisherSigning.h>
#include <drs/engine/PackagePublisherTrustStore.h>
#include <drs/engine/SecureBuffer.h>

#include <cstdint>
#include <string>

namespace drs::signing
{
struct PackageSigningAuditEvent
{
    std::string auditId;
    std::string signingKeyId;
    std::string canonicalDigestHex;
    std::uint64_t canonicalBytes = 0;
    std::string eventUtc;
};

class PackageSigningAuditSink
{
public:
    virtual ~PackageSigningAuditSink() = default;
    virtual bool recordPackageSigningEvent(
        const PackageSigningAuditEvent& event) const = 0;
};

// Intended for a controlled CI/service process only. This target is not linked
// into the plugin, standalone application, or engine adapter.
class ControlledPackageSigner final
    : public drs::engine::PackagePublisherSigningClient
{
public:
    ControlledPackageSigner(std::string signingKeyId,
                            drs::engine::PackageSigningKeyState keyState,
                            drs::engine::SecureBuffer privateKey,
                            const PackageSigningAuditSink& auditSink);

    bool signCanonicalPackage(
        const drs::engine::PackagePublisherSigningRequest& request,
        drs::engine::PackagePublisherSigningResponse& response,
        std::string& issue) const override;

private:
    std::string signingKeyId_;
    drs::engine::PackageSigningKeyState keyState_;
    drs::engine::SecureBuffer privateKey_;
    const PackageSigningAuditSink& auditSink_;
};
} // namespace drs::signing
