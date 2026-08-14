#include "drs/engine/PlayableInstrumentLicense.h"
#include "shared/ProjectStorage.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

struct TemporaryDirectory
{
    explicit TemporaryDirectory(juce::File directory) : root(std::move(directory)) {}
    ~TemporaryDirectory() { root.deleteRecursively(); }

    juce::File root;
};

void writeBytes(const juce::File& file, const std::vector<std::uint8_t>& bytes)
{
    require(file.getParentDirectory().createDirectory() || file.getParentDirectory().isDirectory(),
            "Could not create a license test fixture directory.");
    require(file.replaceWithData(bytes.data(), bytes.size()),
            "Could not write a license test fixture.");
}

std::vector<std::uint8_t> readBytes(const juce::File& file)
{
    juce::MemoryBlock data;
    require(file.loadFileAsData(data), "Could not read imported license bytes.");
    const auto* begin = static_cast<const std::uint8_t*>(data.getData());
    return { begin, begin + data.getSize() };
}

void requirePreserved(const juce::File& target,
                      const std::vector<std::uint8_t>& expected,
                      const std::string& message)
{
    require(target.existsAsFile() && readBytes(target) == expected, message);
}
} // namespace

int main()
{
    try
    {
        const auto root = juce::File::getSpecialLocation(juce::File::tempDirectory)
                              .getChildFile("drs-license-li02-" + juce::Uuid().toString());
        TemporaryDirectory cleanup(root);
        require(root.createDirectory(), "Could not create the LI-02 test root.");

        const auto projectDirectory = root.getChildFile("Instrument");
        const auto sourceDirectory = root.getChildFile("Sources");
        require(projectDirectory.createDirectory() && sourceDirectory.createDirectory(),
                "Could not create LI-02 test directories.");

        const auto projectFile = projectDirectory.getChildFile("Instrument.drsproj");
        require(projectFile.replaceWithText("{}"), "Could not create the saved project fixture.");

        const auto targetFile = drs::app::getProjectLicenseFile(projectFile);
        require(targetFile == projectDirectory.getChildFile("LICENSE.txt"),
                "The license destination must be LICENSE.txt at the instrument root.");
        require(drs::app::getProjectLicenseFile({}) == juce::File(),
                "An unsaved project must not resolve a license destination.");

        const std::vector<std::uint8_t> originalBytes {
            'C', 'o', 'p', 'y', 'r', 'i', 'g', 'h', 't', ' ', '2', '0', '2', '6', '\r', '\n'
        };
        const auto originalSource = sourceDirectory.getChildFile("license.txt");
        writeBytes(originalSource, originalBytes);

        const auto initialImport = drs::app::importProjectLicenseFile(originalSource, projectFile);
        require(initialImport.imported && initialImport.targetFile == targetFile,
                "A valid UTF-8 .txt license should import successfully.");
        requirePreserved(targetFile, originalBytes,
                         "License import must preserve the selected bytes exactly.");

        const auto sameSourceImport = drs::app::importProjectLicenseFile(targetFile, projectFile);
        require(sameSourceImport.imported,
                "Selecting the canonical project license itself should be a successful no-op.");
        requirePreserved(targetFile, originalBytes,
                         "Same-source import must not rewrite the canonical license.");

        const auto invalidExtension = sourceDirectory.getChildFile("license.md");
        writeBytes(invalidExtension, { 'v', 'a', 'l', 'i', 'd' });
        const auto invalidExtensionResult = drs::app::importProjectLicenseFile(invalidExtension, projectFile);
        require(!invalidExtensionResult.imported
                    && invalidExtensionResult.errorMessage.containsIgnoreCase(".txt"),
                "A non-.txt license must be rejected with an actionable extension error.");
        requirePreserved(targetFile, originalBytes,
                         "Extension validation failure must preserve the existing license.");

        const auto missingResult = drs::app::importProjectLicenseFile(
            sourceDirectory.getChildFile("missing.txt"), projectFile);
        require(!missingResult.imported
                    && missingResult.errorMessage.containsIgnoreCase("does not exist"),
                "A missing license must be rejected with an actionable error.");

        std::vector<std::uint8_t> oversizedBytes(
            static_cast<std::size_t>(drs::engine::maximumPlayableInstrumentLicenseBytes) + 1u,
            static_cast<std::uint8_t>('x'));
        const auto oversizedSource = sourceDirectory.getChildFile("oversized.txt");
        writeBytes(oversizedSource, oversizedBytes);
        const auto oversizedResult = drs::app::importProjectLicenseFile(oversizedSource, projectFile);
        require(!oversizedResult.imported
                    && oversizedResult.errorMessage.containsIgnoreCase("1 MiB"),
                "A license larger than 1 MiB must be rejected.");
        requirePreserved(targetFile, originalBytes,
                         "Size validation failure must preserve the existing license.");

        const auto invalidUtf8Source = sourceDirectory.getChildFile("invalid-utf8.txt");
        writeBytes(invalidUtf8Source, { 0xc3u, 0x28u });
        const auto invalidUtf8Result = drs::app::importProjectLicenseFile(invalidUtf8Source, projectFile);
        require(!invalidUtf8Result.imported
                    && invalidUtf8Result.errorMessage.containsIgnoreCase("UTF-8"),
                "Malformed UTF-8 must be rejected.");
        requirePreserved(targetFile, originalBytes,
                         "UTF-8 validation failure must preserve the existing license.");

        const auto embeddedNullSource = sourceDirectory.getChildFile("embedded-null.txt");
        writeBytes(embeddedNullSource, { 'a', 0u, 'b' });
        const auto embeddedNullResult = drs::app::importProjectLicenseFile(embeddedNullSource, projectFile);
        require(!embeddedNullResult.imported
                    && embeddedNullResult.errorMessage.containsIgnoreCase("NUL"),
                "Embedded NUL bytes must be rejected.");
        requirePreserved(targetFile, originalBytes,
                         "NUL validation failure must preserve the existing license.");

        const auto binaryControlSource = sourceDirectory.getChildFile("binary-control.txt");
        writeBytes(binaryControlSource, { 'a', 0x01u, 'b' });
        const auto binaryControlResult = drs::app::importProjectLicenseFile(binaryControlSource, projectFile);
        require(!binaryControlResult.imported
                    && binaryControlResult.errorMessage.containsIgnoreCase("binary control"),
                "Binary control bytes must be rejected.");
        requirePreserved(targetFile, originalBytes,
                         "Binary-content validation failure must preserve the existing license.");

        const auto replacementSource = sourceDirectory.getChildFile("replacement.txt");
        const std::vector<std::uint8_t> replacementBytes { 'r', 'e', 'p', 'l', 'a', 'c', 'e' };
        writeBytes(replacementSource, replacementBytes);
        drs::app::ProjectLicenseFileImportOptions interruptedOptions;
        interruptedOptions.allowCommitAtCheckpoint = [](drs::app::ProjectLicenseFileImportCheckpoint)
        {
            return false;
        };
        const auto interruptedResult = drs::app::importProjectLicenseFile(
            replacementSource, projectFile, interruptedOptions);
        require(!interruptedResult.imported
                    && interruptedResult.errorMessage.containsIgnoreCase("preserved"),
                "An interrupted replacement must report that the existing license was preserved.");
        requirePreserved(targetFile, originalBytes,
                         "An interrupted atomic replacement must leave the previous license intact.");

        const std::vector<std::uint8_t> bomBytes {
            0xefu, 0xbbu, 0xbfu, 'L', 'i', 'c', 'e', 'n', 's', 'e', '\n'
        };
        const auto bomSource = sourceDirectory.getChildFile("LICENSE.TXT");
        writeBytes(bomSource, bomBytes);
        const auto bomResult = drs::app::importProjectLicenseFile(bomSource, projectFile);
        require(bomResult.imported, "A UTF-8 BOM and uppercase .TXT extension must be accepted.");
        requirePreserved(targetFile, bomBytes,
                         "Accepted BOM bytes must be preserved exactly.");

        const auto unsavedResult = drs::app::importProjectLicenseFile(originalSource, {});
        require(!unsavedResult.imported
                    && unsavedResult.errorMessage.containsIgnoreCase("Save the project"),
                "Shared storage must reject license import for an unsaved project.");

        std::vector<std::uint8_t> maximumBytes(
            static_cast<std::size_t>(drs::engine::maximumPlayableInstrumentLicenseBytes),
            static_cast<std::uint8_t>('m'));
        const auto maximumSource = sourceDirectory.getChildFile("maximum.txt");
        writeBytes(maximumSource, maximumBytes);
        const auto maximumResult = drs::app::importProjectLicenseFile(maximumSource, projectFile);
        require(maximumResult.imported,
                "A license exactly at the 1 MiB boundary must be accepted.");
        requirePreserved(targetFile, maximumBytes,
                         "The maximum accepted license must remain byte-identical.");

        std::cout << "Playable instrument license LI-02 project import tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Playable instrument license LI-02 project import tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
