#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace drs::engine
{
struct HiseProjectDirectorySnapshot
{
    std::string name;
    std::string relativePath;
    std::string absolutePath;
    bool exists = false;
    std::size_t matchingFileCount = 0;
};

struct HiseProjectContentSnapshot
{
    bool repoContentRootExists = false;
    std::string repoRoot;
    std::string repoContentRoot;
    std::string runtimeAppDataRoot;
    std::vector<HiseProjectDirectorySnapshot> repoDirectories;
    std::vector<HiseProjectDirectorySnapshot> runtimeDirectories;
    std::size_t presetFileCount = 0;
    std::size_t sampleMapFileCount = 0;
};

HiseProjectContentSnapshot getHiseProjectContentSnapshot();
} // namespace drs::engine
