#pragma once

#include "drs/engine/AuthoringPreviewContract.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace drs::engine
{
enum class AuthoringPreviewCommandType : std::uint8_t
{
    noteOn = 0,
    noteOff,
    stopAll,
    emergencyReset,
    auditionSelectedZone,
    auditionSelectedGroup,
    auditionCurrentDraft
};

enum class AuthoringPreviewEventType : std::uint8_t
{
    noteOn = 0,
    noteOff,
    allNotesOff,
    reset
};

struct AuthoringPreviewCommand
{
    AuthoringPreviewCommandType type = AuthoringPreviewCommandType::noteOn;
    AuthoringPreviewAuditionSource source = AuthoringPreviewAuditionSource::authoringKeyboard;
    int midiNote = 60;
    float velocity = 1.0f;
    std::uint32_t sampleOffset = 0;
    bool emitNote = true;
    std::string selectedZoneId;
    std::string selectedGroupId;
};

struct AuthoringPreviewEventCommand
{
    AuthoringPreviewEventType type = AuthoringPreviewEventType::noteOn;
    int midiNote = 60;
    float velocity = 1.0f;
    std::uint32_t sampleOffset = 0;
};

struct AuthoringPreviewCommandDispatch
{
    bool accepted = false;
    bool preparationRequested = false;
    AuthoringPreviewScope requestedScope = AuthoringPreviewScope::selectedZone;
    bool hasEvent = false;
    AuthoringPreviewEventCommand event;
    const char* rejectionCode = nullptr;
};

struct AuthoringPreviewCommandAdapterSnapshot
{
    std::size_t ownedNoteCount = 0;
    std::size_t distinctOwnedNoteCount = 0;
    std::uint64_t acceptedCommandCount = 0;
    std::uint64_t rejectedCommandCount = 0;
    std::uint64_t emittedEventCount = 0;
    std::uint64_t suppressedNoteOffCount = 0;
    std::uint64_t stopCount = 0;
    std::uint64_t resetCount = 0;
};

class AuthoringPreviewCommandAdapter final
{
public:
    AuthoringPreviewCommandDispatch dispatch(const AuthoringPreviewCommand& command) noexcept;
    AuthoringPreviewCommandAdapterSnapshot getSnapshot() const noexcept;
    void clearOwnership() noexcept;

private:
    static constexpr std::size_t sourceCount = 4;
    static constexpr std::size_t noteCount = 128;

    static std::size_t sourceIndex(AuthoringPreviewAuditionSource source) noexcept;
    AuthoringPreviewCommandDispatch dispatchNoteOn(const AuthoringPreviewCommand& command) noexcept;
    AuthoringPreviewCommandDispatch dispatchNoteOff(const AuthoringPreviewCommand& command) noexcept;
    void countAccepted(AuthoringPreviewCommandDispatch& dispatch) noexcept;
    void countRejected(AuthoringPreviewCommandDispatch& dispatch, const char* code) noexcept;

    std::array<std::array<std::uint16_t, noteCount>, sourceCount> ownershipBySource {};
    std::array<std::uint16_t, noteCount> totalOwnershipByNote {};
    AuthoringPreviewCommandAdapterSnapshot counters;
};
} // namespace drs::engine
