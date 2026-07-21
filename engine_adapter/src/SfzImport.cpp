#include "drs/engine/SfzImport.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>

namespace drs::engine
{
namespace
{
namespace fs = std::filesystem;

void addFinding(std::vector<SfzImportFinding>& findings,
                SfzImportFindingSeverity severity,
                SfzImportSupportDisposition disposition,
                const std::string& code,
                const std::string& summary,
                const std::string& detail,
                const SfzImportSourceLocation& location = {})
{
    SfzImportFinding finding;
    finding.severity = severity;
    finding.disposition = disposition;
    finding.code = code;
    finding.summary = summary;
    finding.detail = detail;
    finding.location = location;
    findings.push_back(std::move(finding));
}

std::string toLowerAscii(const std::string& text)
{
    std::string lowered = text;
    std::transform(lowered.begin(),
                   lowered.end(),
                   lowered.begin(),
                   [](unsigned char character)
                   {
                       return static_cast<char>(std::tolower(character));
                   });
    return lowered;
}

std::string trimAscii(const std::string& text)
{
    auto begin = text.begin();
    while (begin != text.end() && std::isspace(static_cast<unsigned char>(*begin)) != 0)
        ++begin;

    auto end = text.end();
    while (end != begin && std::isspace(static_cast<unsigned char>(*(end - 1))) != 0)
        --end;

    return std::string(begin, end);
}

std::string toDisplayPath(const fs::path& path)
{
    return path.lexically_normal().generic_string();
}

std::string stripLineComment(const std::string& line)
{
    auto inQuotes = false;

    for (std::size_t index = 0; index < line.size(); ++index)
    {
        const auto character = line[index];
        if (character == '"' && (index == 0 || line[index - 1] != '\\'))
            inQuotes = !inQuotes;

        if (!inQuotes && character == '/' && index + 1 < line.size() && line[index + 1] == '/')
            return line.substr(0, index);
    }

    return line;
}

SfzOpcodeScope scopeFromHeaderName(const std::string& headerName)
{
    const auto lowered = toLowerAscii(trimAscii(headerName));

    if (lowered == "control")
        return SfzOpcodeScope::control;
    if (lowered == "global")
        return SfzOpcodeScope::global;
    if (lowered == "master")
        return SfzOpcodeScope::master;
    if (lowered == "group")
        return SfzOpcodeScope::group;
    if (lowered == "region")
        return SfzOpcodeScope::region;
    if (lowered == "curve")
        return SfzOpcodeScope::curve;
    if (lowered == "effect")
        return SfzOpcodeScope::effect;
    if (lowered == "midi")
        return SfzOpcodeScope::midi;
    if (lowered == "sample")
        return SfzOpcodeScope::sample;

    return SfzOpcodeScope::unknown;
}

void addSourceFile(SfzParsedDocument& document, const fs::path& path)
{
    const auto displayPath = toDisplayPath(path);
    if (std::find(document.sourceFiles.begin(), document.sourceFiles.end(), displayPath)
        == document.sourceFiles.end())
    {
        document.sourceFiles.push_back(displayPath);
    }
}

bool parseIncludeDirective(const std::string& line, std::string& includePath)
{
    const auto trimmed = trimAscii(line);
    if (trimmed.rfind("#include", 0) != 0)
        return false;

    auto remainder = trimAscii(trimmed.substr(8));
    if (remainder.empty())
        return false;

    if (!remainder.empty() && remainder.front() == '"' && remainder.back() == '"' && remainder.size() >= 2)
    {
        includePath = remainder.substr(1, remainder.size() - 2);
        return true;
    }

    includePath = remainder;
    return true;
}

using ResolvedOpcodeMap = std::map<std::string, SfzResolvedOpcode>;

ResolvedOpcodeMap buildLocalOpcodeMap(const SfzParsedSection& section)
{
    ResolvedOpcodeMap map;
    for (const auto& opcode : section.opcodes)
    {
        map[opcode.name] = { opcode.name, opcode.value, opcode.location, false };
    }
    return map;
}

void overlayOpcodes(ResolvedOpcodeMap& destination, const ResolvedOpcodeMap& source, bool inherited)
{
    for (const auto& [name, opcode] : source)
    {
        auto resolved = opcode;
        resolved.inherited = inherited;
        destination[name] = std::move(resolved);
    }
}

std::vector<SfzResolvedOpcode> toResolvedOpcodeVector(const ResolvedOpcodeMap& map)
{
    std::vector<SfzResolvedOpcode> result;
    result.reserve(map.size());
    for (const auto& [_, opcode] : map)
        result.push_back(opcode);
    return result;
}

bool parseFileRecursive(const fs::path& filePath,
                        SfzParsedDocument& document,
                        std::vector<SfzImportFinding>& findings,
                        std::vector<fs::path>& includeStack,
                        std::size_t& currentSectionIndex,
                        std::size_t& nextDocumentOrder)
{
    const auto normalizedPath = filePath.lexically_normal();
    const auto cycleIterator = std::find(includeStack.begin(), includeStack.end(), normalizedPath);
    if (cycleIterator != includeStack.end())
    {
        addFinding(findings,
                   SfzImportFindingSeverity::error,
                   SfzImportSupportDisposition::blocking,
                   "include.cycle",
                   "Include cycle detected",
                   "SFZ include recursion re-entered '" + toDisplayPath(normalizedPath) + "'.",
                   { toDisplayPath(normalizedPath), 0, 0, SfzOpcodeScope::unknown, "#include" });
        return false;
    }

    std::ifstream input(normalizedPath, std::ios::binary);
    if (!input.good())
    {
        addFinding(findings,
                   SfzImportFindingSeverity::error,
                   SfzImportSupportDisposition::blocking,
                   "source.missing",
                   "SFZ source file missing",
                   "Could not open SFZ source file '" + toDisplayPath(normalizedPath) + "'.",
                   { toDisplayPath(normalizedPath), 0, 0, SfzOpcodeScope::unknown, "" });
        return false;
    }

    addSourceFile(document, normalizedPath);
    includeStack.push_back(normalizedPath);

    auto complete = true;
    std::string rawLine;
    std::size_t lineNumber = 0;

    while (std::getline(input, rawLine))
    {
        ++lineNumber;
        if (!rawLine.empty() && rawLine.back() == '\r')
            rawLine.pop_back();

        const auto line = stripLineComment(rawLine);
        const auto trimmed = trimAscii(line);
        if (trimmed.empty())
            continue;

        std::string includePath;
        if (parseIncludeDirective(line, includePath))
        {
            const auto resolvedIncludePath = (normalizedPath.parent_path() / fs::path(includePath)).lexically_normal();
            const auto includeComplete = parseFileRecursive(resolvedIncludePath,
                                                            document,
                                                            findings,
                                                            includeStack,
                                                            currentSectionIndex,
                                                            nextDocumentOrder);
            complete = includeComplete && complete;
            continue;
        }

        std::size_t index = 0;
        while (index < line.size())
        {
            while (index < line.size() && std::isspace(static_cast<unsigned char>(line[index])) != 0)
                ++index;

            if (index >= line.size())
                break;

            if (line[index] == '<')
            {
                const auto headerEnd = line.find('>', index + 1);
                if (headerEnd == std::string::npos)
                {
                    addFinding(findings,
                               SfzImportFindingSeverity::error,
                               SfzImportSupportDisposition::blocking,
                               "syntax.header_unterminated",
                               "SFZ header was not closed",
                               "Encountered an unterminated SFZ header.",
                               { toDisplayPath(normalizedPath), lineNumber, index + 1, SfzOpcodeScope::unknown, "" });
                    complete = false;
                    break;
                }

                auto headerName = trimAscii(line.substr(index + 1, headerEnd - index - 1));
                headerName = toLowerAscii(headerName);

                SfzParsedSection section;
                section.scope = scopeFromHeaderName(headerName);
                section.headerName = headerName;
                section.headerLocation = { toDisplayPath(normalizedPath), lineNumber, index + 1, section.scope, headerName };
                section.documentOrder = nextDocumentOrder++;
                document.sections.push_back(std::move(section));
                currentSectionIndex = document.sections.empty() ? 0 : document.sections.size() - 1;

                if (document.sections.back().scope == SfzOpcodeScope::unknown)
                {
                    addFinding(findings,
                               SfzImportFindingSeverity::warning,
                               SfzImportSupportDisposition::reportedOnly,
                               "header.unknown",
                               "Unknown SFZ header",
                               "The parser preserved unknown header '" + headerName + "' for later compatibility reporting.",
                               document.sections.back().headerLocation);
                }

                index = headerEnd + 1;
                continue;
            }

            const auto keyStart = index;
            while (index < line.size()
                   && std::isspace(static_cast<unsigned char>(line[index])) == 0
                   && line[index] != '=')
            {
                ++index;
            }

            const auto keyText = line.substr(keyStart, index - keyStart);
            auto opcodeName = toLowerAscii(trimAscii(keyText));

            while (index < line.size() && std::isspace(static_cast<unsigned char>(line[index])) != 0)
                ++index;

            if (opcodeName.empty() || index >= line.size() || line[index] != '=')
            {
                addFinding(findings,
                           SfzImportFindingSeverity::error,
                           SfzImportSupportDisposition::blocking,
                           "syntax.invalid_token",
                           "Invalid SFZ token",
                           "Encountered token '" + trimAscii(keyText) + "' that was not a valid opcode assignment.",
                           { toDisplayPath(normalizedPath), lineNumber, keyStart + 1, SfzOpcodeScope::unknown, trimAscii(keyText) });
                complete = false;

                while (index < line.size() && std::isspace(static_cast<unsigned char>(line[index])) == 0)
                    ++index;
                continue;
            }

            ++index;
            while (index < line.size() && std::isspace(static_cast<unsigned char>(line[index])) != 0)
                ++index;

            if (currentSectionIndex >= document.sections.size())
            {
                addFinding(findings,
                           SfzImportFindingSeverity::error,
                           SfzImportSupportDisposition::blocking,
                           "syntax.opcode_without_header",
                           "Opcode appeared before any SFZ header",
                           "Encountered opcode '" + opcodeName + "' before a valid SFZ header had been declared.",
                           { toDisplayPath(normalizedPath), lineNumber, keyStart + 1, SfzOpcodeScope::unknown, opcodeName });
                complete = false;

                while (index < line.size() && std::isspace(static_cast<unsigned char>(line[index])) == 0)
                    ++index;
                continue;
            }

            std::string opcodeValue;
            if (index < line.size() && line[index] == '"')
            {
                const auto valueStart = index + 1;
                ++index;
                auto closed = false;
                while (index < line.size())
                {
                    if (line[index] == '"' && line[index - 1] != '\\')
                    {
                        closed = true;
                        break;
                    }
                    ++index;
                }

                if (!closed)
                {
                    addFinding(findings,
                               SfzImportFindingSeverity::error,
                               SfzImportSupportDisposition::blocking,
                               "syntax.quoted_value_unterminated",
                               "Quoted SFZ value was not closed",
                               "Encountered an unterminated quoted value for opcode '" + opcodeName + "'.",
                               { toDisplayPath(normalizedPath),
                                 lineNumber,
                                 valueStart,
                                 document.sections[currentSectionIndex].scope,
                                 opcodeName });
                    complete = false;
                    opcodeValue = line.substr(valueStart);
                    index = line.size();
                }
                else
                {
                    opcodeValue = line.substr(valueStart, index - valueStart);
                    ++index;
                }
            }
            else
            {
                const auto valueStart = index;
                while (index < line.size() && std::isspace(static_cast<unsigned char>(line[index])) == 0)
                    ++index;
                opcodeValue = line.substr(valueStart, index - valueStart);
            }

            if (opcodeValue.empty())
            {
                addFinding(findings,
                           SfzImportFindingSeverity::error,
                           SfzImportSupportDisposition::blocking,
                           "syntax.empty_value",
                           "SFZ opcode value was empty",
                           "Encountered opcode '" + opcodeName + "' without a value.",
                           { toDisplayPath(normalizedPath),
                             lineNumber,
                             keyStart + 1,
                             document.sections[currentSectionIndex].scope,
                             opcodeName });
                complete = false;
                continue;
            }

            document.sections[currentSectionIndex].opcodes.push_back(
                { opcodeName,
                  opcodeValue,
                  { toDisplayPath(normalizedPath),
                    lineNumber,
                    keyStart + 1,
                    document.sections[currentSectionIndex].scope,
                    opcodeName } });
        }
    }

    includeStack.pop_back();
    return complete;
}
} // namespace

SfzDocumentParseResult parseSfzDocument(const std::string& sfzPath)
{
    SfzDocumentParseResult result;
    result.document.rootDocumentPath = toDisplayPath(fs::path(sfzPath));
    result.state = "Discovering";

    std::size_t currentSectionIndex = static_cast<std::size_t>(-1);
    std::size_t nextDocumentOrder = 0;
    std::vector<fs::path> includeStack;

    const auto complete = parseFileRecursive(fs::path(sfzPath),
                                             result.document,
                                             result.findings,
                                             includeStack,
                                             currentSectionIndex,
                                             nextDocumentOrder);

    result.parsed = result.findings.end()
        == std::find_if(result.findings.begin(),
                        result.findings.end(),
                        [](const SfzImportFinding& finding)
                        {
                            return finding.severity == SfzImportFindingSeverity::error;
                        });
    result.complete = complete && result.parsed;

    if (!result.parsed)
        result.state = "Blocked";
    else if (!result.complete)
        result.state = "Parsed With Findings";
    else
        result.state = "Parsed";

    return result;
}

SfzDocumentNormalizeResult normalizeSfzDocument(const SfzParsedDocument& document)
{
    SfzDocumentNormalizeResult result;
    result.document.rootDocumentPath = document.rootDocumentPath;
    result.document.sourceFiles = document.sourceFiles;
    result.state = "Normalizing";

    ResolvedOpcodeMap activeControl;
    ResolvedOpcodeMap activeGlobal;
    ResolvedOpcodeMap activeMaster;
    ResolvedOpcodeMap activeGroup;

    result.document.sections.reserve(document.sections.size());

    for (const auto& parsedSection : document.sections)
    {
        const auto localMap = buildLocalOpcodeMap(parsedSection);
        ResolvedOpcodeMap effective;

        switch (parsedSection.scope)
        {
            case SfzOpcodeScope::control:
                overlayOpcodes(activeControl, localMap, false);
                effective = activeControl;
                break;

            case SfzOpcodeScope::global:
                overlayOpcodes(activeGlobal, localMap, false);
                overlayOpcodes(effective, activeControl, true);
                overlayOpcodes(effective, activeGlobal, false);
                break;

            case SfzOpcodeScope::master:
                activeMaster = localMap;
                overlayOpcodes(effective, activeControl, true);
                overlayOpcodes(effective, activeGlobal, true);
                overlayOpcodes(effective, activeMaster, false);
                break;

            case SfzOpcodeScope::group:
                activeGroup = localMap;
                overlayOpcodes(effective, activeControl, true);
                overlayOpcodes(effective, activeGlobal, true);
                overlayOpcodes(effective, activeMaster, true);
                overlayOpcodes(effective, activeGroup, false);
                break;

            case SfzOpcodeScope::region:
                overlayOpcodes(effective, activeControl, true);
                overlayOpcodes(effective, activeGlobal, true);
                overlayOpcodes(effective, activeMaster, true);
                overlayOpcodes(effective, activeGroup, true);
                overlayOpcodes(effective, localMap, false);
                break;

            case SfzOpcodeScope::curve:
            case SfzOpcodeScope::effect:
            case SfzOpcodeScope::midi:
            case SfzOpcodeScope::sample:
            case SfzOpcodeScope::unknown:
                effective = localMap;
                break;
        }

        SfzNormalizedSection normalizedSection;
        normalizedSection.scope = parsedSection.scope;
        normalizedSection.headerName = parsedSection.headerName;
        normalizedSection.headerLocation = parsedSection.headerLocation;
        normalizedSection.documentOrder = parsedSection.documentOrder;
        normalizedSection.localOpcodes = toResolvedOpcodeVector(localMap);
        normalizedSection.effectiveOpcodes = toResolvedOpcodeVector(effective);
        normalizedSection.localOpcodeCount = normalizedSection.localOpcodes.size();
        normalizedSection.inheritedOpcodeCount = normalizedSection.effectiveOpcodes.size() >= normalizedSection.localOpcodeCount
            ? normalizedSection.effectiveOpcodes.size() - normalizedSection.localOpcodeCount
            : 0;
        result.document.sections.push_back(std::move(normalizedSection));
    }

    result.normalized = true;
    result.state = "Normalized";
    return result;
}

const SfzResolvedOpcode* findEffectiveOpcode(const SfzNormalizedSection& section,
                                             const std::string& opcodeName) noexcept
{
    const auto lowered = toLowerAscii(opcodeName);
    const auto iterator = std::find_if(section.effectiveOpcodes.begin(),
                                       section.effectiveOpcodes.end(),
                                       [&](const SfzResolvedOpcode& opcode)
                                       {
                                           return opcode.name == lowered;
                                       });
    return iterator == section.effectiveOpcodes.end() ? nullptr : &(*iterator);
}
} // namespace drs::engine
