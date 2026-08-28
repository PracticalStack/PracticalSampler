#include <drs/signing/ControlledPackageSigner.h>

#include <drs/engine/PackageSignature.h>
#include <drs/engine/PackageV3.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <sodium/utils.h>

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#endif

namespace
{
class FileAuditSink final : public drs::signing::PackageSigningAuditSink
{
public:
    explicit FileAuditSink(std::filesystem::path path) : path_(std::move(path)) {}

    bool recordPackageSigningEvent(
        const drs::signing::PackageSigningAuditEvent& event) const override
    {
        std::ofstream output(path_, std::ios::binary | std::ios::trunc);
        output << "{\n"
               << "  \"auditId\": \"" << event.auditId << "\",\n"
               << "  \"signingKeyId\": \"" << event.signingKeyId << "\",\n"
               << "  \"canonicalDigestSha256\": \"" << event.canonicalDigestHex << "\",\n"
               << "  \"canonicalBytes\": " << event.canonicalBytes << ",\n"
               << "  \"eventUnixMilliseconds\": \"" << event.eventUtc << "\"\n"
               << "}\n";
        return output.good();
    }

private:
    std::filesystem::path path_;
};

} // namespace

int main(int argc, char** argv)
{
    if (argc != 5)
    {
        std::cerr << "Usage: drs_controlled_package_signer <key-id> "
                     "<canonical-signed-region> <signature-output> <audit-output>\n"
                     "The raw 64-byte Ed25519 private key must be supplied on stdin.\n";
        return 2;
    }
#if defined(_WIN32)
    _setmode(_fileno(stdin), _O_BINARY);
#endif
    std::array<std::uint8_t, drs::engine::packageEd25519PrivateKeyBytes + 1u> inputKey {};
    std::cin.read(reinterpret_cast<char*>(inputKey.data()),
                  static_cast<std::streamsize>(inputKey.size()));
    const auto keyBytesRead = static_cast<std::size_t>(std::cin.gcount());
    if (keyBytesRead != drs::engine::packageEd25519PrivateKeyBytes)
    {
        std::cerr << "Controlled signing key input must be exactly 64 raw bytes.\n";
        return 3;
    }
    std::vector<std::uint8_t> keyVector(
        inputKey.begin(), inputKey.begin() + drs::engine::packageEd25519PrivateKeyBytes);
    drs::engine::SecureBuffer privateKey(std::move(keyVector));
    sodium_memzero(inputKey.data(), inputKey.size());

    std::error_code fileError;
    const auto canonicalBytes = std::filesystem::file_size(argv[2], fileError);
    if (fileError || canonicalBytes > drs::engine::packageV3MaximumPackageBytes)
    {
        std::cerr << "Canonical package region is missing, unreadable, or oversized.\n";
        return 4;
    }
    FileAuditSink auditSink(argv[4]);
    drs::signing::ControlledPackageSigner signer(
        argv[1], drs::engine::PackageSigningKeyState::active,
        std::move(privateKey), auditSink);
    drs::engine::PackagePublisherSigningRequest request;
    request.signingKeyId = argv[1];
    request.canonicalSignedFilePath = argv[2];
    request.canonicalSignedBytesLength = canonicalBytes;
    drs::engine::PackagePublisherSigningResponse response;
    std::string issue;
    if (! signer.signCanonicalPackage(request, response, issue))
    {
        std::cerr << issue << '\n';
        return 5;
    }
    std::ofstream signatureOutput(argv[3], std::ios::binary | std::ios::trunc);
    signatureOutput.write(reinterpret_cast<const char*>(response.signature.data()),
                          static_cast<std::streamsize>(response.signature.size()));
    if (! signatureOutput.good())
    {
        std::cerr << "Detached signature output could not be written.\n";
        return 6;
    }
    std::cout << response.auditId << '\n';
    return 0;
}
