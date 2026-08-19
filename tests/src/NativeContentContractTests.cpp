#include "drs/engine/NativeContent.h"
#include "drs/engine/RuntimeLoader.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void requireNativePath(const fs::path& path, const std::string& label)
{
    const auto normalized = path.lexically_normal();
    require(normalized.is_absolute(), label + " must be absolute.");
    require(normalized.generic_string().find("hise_project") == std::string::npos,
            label + " must not retain a legacy authoring-project path.");
}
} // namespace

int main()
{
    try
    {
        const auto roots = drs::engine::getNativeContentRoots();
        const fs::path repositoryRoot(roots.repositoryRoot);
        const fs::path samplesRoot(roots.samplesRoot);
        const fs::path runtimeRoot(roots.runtimeRoot);

        requireNativePath(repositoryRoot, "Native repository root");
        requireNativePath(samplesRoot, "Native samples root");
        requireNativePath(runtimeRoot, "Native runtime root");
        require(samplesRoot == (repositoryRoot / "content" / "samples").lexically_normal(),
                "Native samples root must be repository content/samples.");
        require(runtimeRoot == (repositoryRoot / "content" / "runtime" / "phase1").lexically_normal(),
                "Native runtime root must be repository content/runtime/phase1.");
        require(fs::is_directory(samplesRoot), "Native samples root must exist as a directory.");
        require(fs::is_directory(runtimeRoot), "Native runtime root must exist as a directory.");

        for (const auto* fileName : { "DRS_Sine_A3.wav", "DRS_TriangleLead_A4.wav" })
        {
            const auto samplePath = samplesRoot / fileName;
            requireNativePath(samplePath, "Native sample path");
            require(fs::is_regular_file(samplePath),
                    "Required native sample fixture is missing: " + samplePath.generic_string());
        }

        const auto referenceProject = drs::engine::loadPhase1ReferenceProjectManifest();
        require(referenceProject.loaded,
                "The Phase 1 reference project must load through the native content contract.");
        require(fs::path(referenceProject.project.contentRootPath).lexically_normal() == samplesRoot,
                "Reference project contentRoot must resolve to the native samples root.");
        require(referenceProject.project.sampleSources.size() == 2,
                "Reference project must retain both native sample sources.");
        for (const auto& sampleSource : referenceProject.project.sampleSources)
        {
            requireNativePath(fs::path(sampleSource.path), "Resolved reference sample path");
            require(fs::path(sampleSource.path).parent_path() == samplesRoot,
                    "Resolved reference sample path must remain directly under content/samples.");
            require(fs::is_regular_file(fs::path(sampleSource.path)),
                    "Resolved reference sample must exist: " + sampleSource.path);
        }

        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Native content contract tests failed: " << exception.what() << '\n';
        return 1;
    }
}
