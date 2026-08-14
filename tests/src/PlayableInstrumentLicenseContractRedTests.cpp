#include <array>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
namespace fs = std::filesystem;

constexpr std::array<std::string_view, 6> redSeams {
    "project-import-storage",
    "package-export-persistence",
    "package-reader-integrity",
    "activation-ownership",
    "plugin-menu-viewer",
    "standalone-menu-viewer"
};

bool isKnownSeam(const std::string_view value)
{
    for (const auto seam : redSeams)
        if (seam == value)
            return true;
    return false;
}

std::string readText(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Could not read " + path.generic_string());
    return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

bool containsAll(const std::string& source,
                 const std::initializer_list<std::string_view> markers)
{
    for (const auto marker : markers)
        if (source.find(marker) == std::string::npos)
            return false;
    return true;
}

bool seamImplemented(const fs::path& root, const std::string_view seam)
{
    if (seam == "project-import-storage")
    {
        return containsAll(readText(root / "app/src/shared/ProjectStorage.cpp"),
                           { "importProjectLicenseFile(", "playableInstrumentLicenseFileName",
                             "maximumPlayableInstrumentLicenseBytes" });
    }
    if (seam == "package-export-persistence")
    {
        return containsAll(readText(root / "app/src/shared/PerformancePackageExportService.cpp"),
                           { "resolveProjectLicense", "license.payloadId", "licenseText" })
            && containsAll(readText(root / "engine_adapter/src/PackageV2StreamingExport.cpp"),
                           { "PerformancePackagePayloadKind::licenseText",
                             "PackageV2RecordKind::licenseText" });
    }
    if (seam == "package-reader-integrity")
    {
        return containsAll(readText(root / "engine_adapter/src/PackageReader.cpp"),
                           { "manifest.license.payloadId", "result.licenseText",
                             "maximumPlayableInstrumentLicenseBytes" })
            && containsAll(readText(root / "engine_adapter/src/PackageReaderDispatch.cpp"),
                           { "PackageV2RecordKind::licenseText", "maximumPlayableInstrumentLicenseBytes" });
    }
    if (seam == "activation-ownership")
    {
        return containsAll(readText(root / "engine_adapter/src/EngineFacade.cpp"),
                           { "packageLicenseText", "licenseText.plaintextBytes" })
            && readText(root / "engine_adapter/include/drs/engine/EngineFacade.h")
                   .find("getPerformancePackageLicenseText") != std::string::npos;
    }
    if (seam == "plugin-menu-viewer")
    {
        return containsAll(readText(root / "app/src/plugin/PluginEditor.cpp"),
                           { "importLicenseFile()", "viewLicense()",
                             "importLicenseFileMenuLabel", "viewLicenseMenuLabel" });
    }
    if (seam == "standalone-menu-viewer")
    {
        return containsAll(readText(root / "app/src/standalone/MainComponent.cpp"),
                           { "importLicenseFile()", "viewLicense()",
                             "importLicenseFileMenuLabel", "viewLicenseMenuLabel" });
    }
    return false;
}
} // namespace

// LI-01 owns contract and expected-red characterization only. Invoke one seam
// directly; each exits 1 until its later slice replaces this audit with a
// registered behavioral regression.
int main(int argc, char** argv)
{
    if (argc != 2 || !isKnownSeam(argv[1]))
    {
        std::cerr << "Usage: drs_playable_instrument_license_contract_red_tests <named-missing-seam>\n";
        for (const auto seam : redSeams)
            std::cerr << "  " << seam << '\n';
        return 2;
    }

    try
    {
        const auto root = fs::path(DRS_SOURCE_ROOT);
        if (seamImplemented(root, argv[1]))
        {
            std::cout << "LI-01 seam implemented: " << argv[1]
                      << ". Promote it to registered green coverage." << std::endl;
            return 0;
        }

        std::cerr << "EXPECTED RED: missing playable instrument license seam '"
                  << argv[1] << "'. Delivery belongs to LI-02 through LI-04."
                  << std::endl;
        return 1;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "LI-01 expected-red audit failed unexpectedly: "
                  << exception.what() << std::endl;
        return 2;
    }
}
