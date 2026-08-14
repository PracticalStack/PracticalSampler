#include "drs/engine/PackageV2.h"
#include "drs/engine/PackageWriter.h"
#include "drs/engine/PerformancePackage.h"
#include "drs/engine/PlayableInstrumentLicense.h"
#include "shared/ProjectStorage.h"
#include "shared/WorkspaceMenuPolicy.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace
{
void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace

int main()
{
    try
    {
        using namespace drs::engine;

        require(std::string(playableInstrumentLicenseFileName) == "LICENSE.txt"
                    && std::string(playableInstrumentLicenseLogicalPath) == "LICENSE.txt",
                "The authored and packaged license path must remain canonical LICENSE.txt.");
        require(std::string(playableInstrumentLicensePayloadId) == "license-text"
                    && std::string(playableInstrumentLicenseMediaType)
                        == "text/plain; charset=utf-8",
                "The playable license payload identity and media type changed.");
        require(maximumPlayableInstrumentLicenseBytes == 1024ull * 1024ull,
                "The playable license source limit must remain 1 MiB.");
        require(playableInstrumentLicenseRequiresUtf8
                    && playableInstrumentLicenseAllowsUtf8Bom
                    && !playableInstrumentLicenseAllowsEmbeddedNull
                    && playableInstrumentLicensePreservesExactBytes,
                "The playable license text-validation policy changed.");
        require(!playableInstrumentLicenseRequiresPackageSchemaBump,
                "The optional license payload must remain additive to package schema 1/2.");
        require(static_cast<std::uint32_t>(PackageV2RecordKind::licenseText)
                    == playableInstrumentLicensePackageV2RecordKind,
                "The package-v2 license record kind must remain reserved at value 7.");
        require(PerformancePackagePayloadKind::licenseText
                    != PerformancePackagePayloadKind::backgroundImage,
                "License text requires a distinct package payload kind.");

        const PerformancePackageManifest manifest;
        require(manifest.schemaVersion == performancePackageLegacySchemaVersion
                    && manifest.minimumReaderSchemaVersion
                        == performancePackageLegacySchemaVersion
                    && manifest.license.payloadId.empty(),
                "Adding an optional license must not promote default package compatibility.");
        require(std::string(drs::app::importLicenseFileMenuLabel)
                    == "Import License File..."
                    && std::string(drs::app::viewLicenseMenuLabel) == "View License",
                "The authoring and Performance File-menu labels changed.");

        static_assert(std::is_default_constructible_v<drs::app::ProjectLicenseFileImportResult>,
                      "The shared project license import result must be default constructible.");

        std::cout << "Playable instrument license LI-01 contract passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Playable instrument license LI-01 contract failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
