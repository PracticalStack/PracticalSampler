#include "shared/WavImportService.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

template <typename ComponentType>
ComponentType* requireChild(juce::Component& component, const juce::String& componentId)
{
    auto* child = dynamic_cast<ComponentType*>(component.findChildWithID(componentId));
    require(child != nullptr,
            "Could not find expected WAV import progress child component: " + componentId.toStdString());
    return child;
}
} // namespace

int main()
{
    try
    {
        int cancelCount = 0;
        drs::app::WavImportProgressComponent component([&cancelCount]
        {
            ++cancelCount;
        });
        component.setSize(720, 64);
        component.resized();
        require(!component.isVisible(),
                "WAV import progress should be hidden before a batch becomes active.");

        drs::app::WavImportBatchSnapshot activeSnapshot;
        activeSnapshot.stage = drs::app::WavImportBatchStage::staging;
        activeSnapshot.status = "Staging WAV import sources";
        activeSnapshot.totalItemCount = 3;
        activeSnapshot.completedItemCount = 1;
        activeSnapshot.successfulItemCount = 1;
        activeSnapshot.warningItemCount = 1;
        activeSnapshot.totalBytesProcessed = 2048;
        activeSnapshot.totalBytesExpected = 4096;

        drs::app::WavImportItemProgress readyItem;
        readyItem.itemId = "wav-1";
        readyItem.sourcePath = "E:/Fixtures/Kick.wav";
        readyItem.stage = drs::app::WavImportItemStage::ready;

        drs::app::WavImportItemProgress activeItem;
        activeItem.itemId = "wav-2";
        activeItem.sourcePath = "E:/Fixtures/Snare.wav";
        activeItem.stage = drs::app::WavImportItemStage::staging;

        activeSnapshot.items = { readyItem, activeItem };
        component.update(activeSnapshot);

        auto* statusLabel = requireChild<juce::Label>(component, "wavImportProgressLabel");
        auto* detailLabel = requireChild<juce::Label>(component, "wavImportProgressDetailLabel");
        auto* cancelButton = requireChild<juce::TextButton>(component, "wavImportProgressCancelButton");

        require(component.isVisible(),
                "Active WAV import progress should remain modelessly visible while work is in flight.");
        require(statusLabel->getText().containsIgnoreCase("Staging")
                    && statusLabel->getText().containsIgnoreCase("Snare.wav"),
                "The WAV progress headline should surface the current batch stage and item.");
        require(detailLabel->getText().containsIgnoreCase("Current:")
                    && detailLabel->getText().containsIgnoreCase("1/3")
                    && detailLabel->getText().containsIgnoreCase("Bytes:"),
                "The WAV progress detail line should surface current item, item counts, and byte counts.");
        require(cancelButton->isEnabled(),
                "The WAV progress cancel button should stay enabled while a batch is active.");

        require(static_cast<bool>(cancelButton->onClick),
                "The WAV progress cancel button should install a cancel handler.");
        cancelButton->onClick();
        require(cancelCount == 1,
                "The WAV progress cancel button should call its shared cancel callback.");

        auto partialSnapshot = activeSnapshot;
        partialSnapshot.stage = drs::app::WavImportBatchStage::completed;
        partialSnapshot.terminalDisposition = drs::app::WavImportTerminalDisposition::partiallyCompleted;
        partialSnapshot.completedItemCount = 3;
        partialSnapshot.successfulItemCount = 2;
        partialSnapshot.failedItemCount = 1;
        partialSnapshot.totalBytesProcessed = partialSnapshot.totalBytesExpected;
        activeItem.stage = drs::app::WavImportItemStage::failed;
        partialSnapshot.items = { readyItem, activeItem };
        component.update(partialSnapshot);

        require(statusLabel->getText().containsIgnoreCase("partially complete"),
                "Terminal partial WAV batches should surface a distinct partial-complete state.");
        require(!cancelButton->isEnabled(),
                "The WAV progress cancel button should disable once the batch reaches a terminal state.");
        require(!component.isVisible(),
                "Completed WAV batches should release their modeless layout space immediately.");

        auto canceledSnapshot = partialSnapshot;
        canceledSnapshot.stage = drs::app::WavImportBatchStage::canceled;
        canceledSnapshot.terminalDisposition = drs::app::WavImportTerminalDisposition::canceled;
        component.update(canceledSnapshot);
        require(statusLabel->getText().containsIgnoreCase("canceled"),
                "Canceled WAV batches should surface a distinct canceled state.");
        require(!component.isVisible(),
                "Canceled WAV batches should not retain modeless layout space.");

        auto failedSnapshot = partialSnapshot;
        failedSnapshot.stage = drs::app::WavImportBatchStage::failed;
        failedSnapshot.terminalDisposition = drs::app::WavImportTerminalDisposition::failed;
        component.update(failedSnapshot);
        require(statusLabel->getText().containsIgnoreCase("failed"),
                "Failed WAV batches should surface a distinct failed state.");
        require(!component.isVisible(),
                "Failed WAV batches should not retain modeless layout space.");

        auto consumedSnapshot = failedSnapshot;
        consumedSnapshot.stage = drs::app::WavImportBatchStage::consumed;
        component.update(consumedSnapshot);
        require(!component.isVisible(),
                "Consumed WAV batches should hide the modeless progress component.");

        std::cout << "WAV import progress tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "WAV import progress tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
