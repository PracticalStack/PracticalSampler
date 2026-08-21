#pragma once

#include <string>
#include <vector>

namespace drs::app::authoring
{
enum class StructureSelectionKind
{
    none,
    layer,
    group,
    zone
};

// UI-only, stable-ID selection state shared by the Structure viewer and the Map
// adapter. It deliberately owns no project object pointers and is safe to
// reconcile after a document refresh or topology mutation.
class AuthoringStructureSelection
{
public:
    StructureSelectionKind getKind() const noexcept { return kind; }
    const std::vector<std::string>& getIds() const noexcept { return ids; }
    const std::string& getPrimaryId() const noexcept { return primaryId; }
    bool isEmpty() const noexcept { return ids.empty(); }
    bool contains(const std::string& id) const noexcept;

    void clear() noexcept;
    void replace(StructureSelectionKind nextKind,
                 std::vector<std::string> nextIds,
                 std::string nextPrimaryId = {});
    void setPrimary(const std::string& id) noexcept;
    void toggle(const std::string& id,
                StructureSelectionKind itemKind);

    // Select a contiguous range in the supplied visible order. With additive
    // true, the range is merged into the current same-kind selection.
    void selectRange(const std::vector<std::string>& orderedIds,
                     const std::string& targetId,
                     StructureSelectionKind itemKind,
                     bool additive = false);

    // Retain only IDs that still exist. The first surviving ID is the
    // deterministic primary fallback when the old primary disappears.
    void reconcile(const std::vector<std::string>& validIds);

private:
    static std::vector<std::string> normalize(std::vector<std::string> values);
    void ensurePrimaryIsSelected() noexcept;

    StructureSelectionKind kind = StructureSelectionKind::none;
    std::vector<std::string> ids;
    std::string primaryId;
};
} // namespace drs::app::authoring
