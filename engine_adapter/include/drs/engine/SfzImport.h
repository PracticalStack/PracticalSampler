#pragma once

#include "drs/engine/SfzImportContract.h"

#include <cstddef>
#include <string>
#include <vector>

namespace drs::engine
{
struct SfzParsedOpcode
{
    std::string name;
    std::string value;
    SfzImportSourceLocation location;
    std::string resolutionBasePath;
};

struct SfzParsedSection
{
    SfzOpcodeScope scope = SfzOpcodeScope::unknown;
    std::string headerName;
    SfzImportSourceLocation headerLocation;
    std::vector<SfzParsedOpcode> opcodes;
    std::size_t documentOrder = 0;
};

struct SfzParsedDocument
{
    std::string rootDocumentPath;
    std::vector<std::string> sourceFiles;
    std::vector<SfzParsedSection> sections;
};

struct SfzDocumentParseResult
{
    bool parsed = false;
    bool complete = false;
    std::string state;
    std::vector<SfzImportFinding> findings;
    std::size_t suppressedFindingCount = 0;
    SfzParsedDocument document;
    SfzImportExecutionState execution;
};

struct SfzResolvedOpcode
{
    std::string name;
    std::string value;
    SfzImportSourceLocation location;
    bool inherited = false;
    std::string resolutionBasePath;
};

struct SfzNormalizedSection
{
    SfzOpcodeScope scope = SfzOpcodeScope::unknown;
    std::string headerName;
    SfzImportSourceLocation headerLocation;
    std::vector<SfzResolvedOpcode> localOpcodes;
    std::vector<SfzResolvedOpcode> effectiveOpcodes;
    std::size_t documentOrder = 0;
    std::size_t inheritedOpcodeCount = 0;
    std::size_t localOpcodeCount = 0;
};

struct SfzNormalizedDocument
{
    std::string rootDocumentPath;
    std::vector<std::string> sourceFiles;
    std::vector<SfzNormalizedSection> sections;
};

struct SfzDocumentNormalizeResult
{
    bool normalized = false;
    std::string state;
    std::vector<SfzImportFinding> findings;
    SfzNormalizedDocument document;
    SfzImportExecutionState execution;
};

SfzDocumentParseResult parseSfzDocument(const std::string& sfzPath);
SfzDocumentParseResult parseSfzDocument(const std::string& sfzPath,
                                        const SfzImportExecutionContext& context);
SfzDocumentNormalizeResult normalizeSfzDocument(const SfzParsedDocument& document);
SfzDocumentNormalizeResult normalizeSfzDocument(const SfzParsedDocument& document,
                                                const SfzImportExecutionContext& context);
const SfzResolvedOpcode* findEffectiveOpcode(const SfzNormalizedSection& section,
                                             const std::string& opcodeName) noexcept;
} // namespace drs::engine
