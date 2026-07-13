#include "shared/PerformanceBankImport.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>

namespace drs::app
{
namespace
{
struct ActivePhraseNote
{
    double startBeat = 0.0;
    int velocity = 96;
};

double normalizeBeatValue(double beatValue)
{
    constexpr double grid = 0.25;
    return std::round(beatValue / grid) * grid;
}

std::string describeChord(const std::vector<int>& midiNotes)
{
    if (midiNotes.empty())
        return "No chord";

    std::vector<int> pitchClasses;
    pitchClasses.reserve(midiNotes.size());
    for (const auto midiNote : midiNotes)
        pitchClasses.push_back((midiNote % 12 + 12) % 12);

    std::sort(pitchClasses.begin(), pitchClasses.end());
    pitchClasses.erase(std::unique(pitchClasses.begin(), pitchClasses.end()), pitchClasses.end());

    static constexpr const char* noteNames[12]
        = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};

    const auto root = pitchClasses.front();
    const auto has = [&](int interval)
    {
        return std::find(pitchClasses.begin(),
                         pitchClasses.end(),
                         (root + interval) % 12)
            != pitchClasses.end();
    };

    std::string quality = "cluster";
    if (has(4) && has(7))
        quality = "major";
    else if (has(3) && has(7))
        quality = "minor";
    else if (has(7))
        quality = "power";
    else if (has(5))
        quality = "sus4";
    else if (has(2))
        quality = "sus2";

    return std::string(noteNames[root]) + " " + quality;
}
} // namespace

MidiPhraseImportResult importMidiPhraseAsset(const std::string& midiFilePath,
                                             const std::string& phraseId,
                                             const std::string& displayName)
{
    MidiPhraseImportResult result;
    result.state = "MIDI import not attempted";

    if (midiFilePath.empty())
    {
        result.state = "MIDI import rejected";
        result.issues.push_back("A MIDI file path is required.");
        return result;
    }

    juce::File midiFile(midiFilePath);
    if (!midiFile.existsAsFile())
    {
        result.state = "MIDI import rejected";
        result.issues.push_back("MIDI file was not found at '" + midiFilePath + "'.");
        return result;
    }

    juce::FileInputStream inputStream(midiFile);
    if (!inputStream.openedOk())
    {
        result.state = "MIDI import rejected";
        result.issues.push_back("MIDI file could not be opened.");
        return result;
    }

    juce::MidiFile midi;
    if (!midi.readFrom(inputStream))
    {
        result.state = "MIDI import rejected";
        result.issues.push_back("MIDI file could not be parsed.");
        return result;
    }

    const auto timeFormat = midi.getTimeFormat();
    if (timeFormat <= 0)
    {
        result.state = "MIDI import rejected";
        result.issues.push_back("Only ticks-per-quarter MIDI files are supported right now.");
        return result;
    }

    drs::engine::RuntimeProjectPhraseAssetDefinition phraseAsset;
    phraseAsset.id = phraseId;
    phraseAsset.displayName = displayName;
    phraseAsset.sourcePath = midiFilePath;
    phraseAsset.ticksPerQuarter = timeFormat;
    phraseAsset.normalizationState = "Normalized to 1/16 note grid";

    std::map<std::tuple<int, int>, ActivePhraseNote> activeNotes;
    std::vector<int> firstChordNotes;
    bool capturedFirstChord = false;
    double maxEndBeat = 0.0;

    for (int trackIndex = 0; trackIndex < midi.getNumTracks(); ++trackIndex)
    {
        if (auto* track = midi.getTrack(trackIndex))
        {
            for (int eventIndex = 0; eventIndex < track->getNumEvents(); ++eventIndex)
            {
                const auto* eventHolder = track->getEventPointer(eventIndex);
                if (eventHolder == nullptr)
                    continue;

                const auto& message = eventHolder->message;
                if (!message.isNoteOnOrOff())
                    continue;

                const auto channel = message.getChannel();
                const auto noteNumber = message.getNoteNumber();
                const auto timestampBeat = static_cast<double>(message.getTimeStamp()) / static_cast<double>(timeFormat);

                if (message.isNoteOn())
                {
                    const auto key = std::make_tuple(channel, noteNumber);
                    if (activeNotes.count(key) > 0)
                        result.issues.push_back("Overlapping note-on detected for MIDI note " + std::to_string(noteNumber) + ".");

                    activeNotes[key] = { timestampBeat, static_cast<int>(message.getVelocity()) };
                    if (!capturedFirstChord && timestampBeat <= 0.001)
                        firstChordNotes.push_back(noteNumber);
                }
                else if (message.isNoteOff())
                {
                    const auto key = std::make_tuple(channel, noteNumber);
                    const auto activeIterator = activeNotes.find(key);
                    if (activeIterator == activeNotes.end())
                    {
                        result.issues.push_back("Note-off without a matching note-on detected for MIDI note "
                                                + std::to_string(noteNumber) + ".");
                        continue;
                    }

                    const auto startBeat = normalizeBeatValue(activeIterator->second.startBeat);
                    const auto durationBeats = normalizeBeatValue(std::max(0.25, timestampBeat - activeIterator->second.startBeat));
                    phraseAsset.notes.push_back({ noteNumber,
                                                  activeIterator->second.velocity,
                                                  startBeat,
                                                  durationBeats });
                    maxEndBeat = std::max(maxEndBeat, startBeat + durationBeats);
                    activeNotes.erase(activeIterator);
                    if (!capturedFirstChord && timestampBeat > 0.001)
                        capturedFirstChord = true;
                }
            }
        }
    }

    if (!activeNotes.empty())
        result.issues.push_back("Some notes were still active at end-of-file and were ignored.");

    if (phraseAsset.notes.empty())
    {
        result.state = "MIDI import rejected";
        result.issues.push_back("No complete note pairs were found in the MIDI file.");
        return result;
    }

    std::sort(phraseAsset.notes.begin(),
              phraseAsset.notes.end(),
              [](const auto& left, const auto& right)
              {
                  if (left.startBeat == right.startBeat)
                      return left.midiNote < right.midiNote;
                  return left.startBeat < right.startBeat;
              });

    phraseAsset.lengthBeats = maxEndBeat;
    phraseAsset.chordHint = describeChord(firstChordNotes);
    phraseAsset.issues = result.issues;

    result.imported = true;
    result.state = result.issues.empty() ? "MIDI phrase imported" : "MIDI phrase imported with warnings";
    result.phraseAsset = std::move(phraseAsset);
    return result;
}
} // namespace drs::app
