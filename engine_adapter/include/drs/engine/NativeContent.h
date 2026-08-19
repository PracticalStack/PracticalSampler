#pragma once

#include <string>

namespace drs::engine
{
struct NativeContentRoots
{
    std::string repositoryRoot;
    std::string samplesRoot;
    std::string runtimeRoot;
};

// Returns the configured product-owned content roots. Paths are absolute and
// use generic separators so callers can safely compare or serialize them.
NativeContentRoots getNativeContentRoots();
} // namespace drs::engine
