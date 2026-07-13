#pragma once

#include "drs/engine/RuntimeModel.h"

#include <string>
#include <vector>

namespace drs::app
{
struct MidiPhraseImportResult
{
    bool imported = false;
    std::string state;
    std::vector<std::string> issues;
    drs::engine::RuntimeProjectPhraseAssetDefinition phraseAsset;
};

MidiPhraseImportResult importMidiPhraseAsset(const std::string& midiFilePath,
                                             const std::string& phraseId,
                                             const std::string& displayName);
} // namespace drs::app
