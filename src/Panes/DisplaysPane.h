#ifndef DISPLAYSPANE_H
#define DISPLAYSPANE_H

#include <Panes/Pane.h>
#include <CZ/AK/Nodes/AKFormSection.h>
#include <CZ/AK/Nodes/AKFormRow.h>
#include <CZ/AK/Nodes/AKRoundSolidColor.h>
#include <CZ/AK/Nodes/AKButton.h>
#include <CZ/AK/Nodes/AKSwitch.h>
#include <CZ/AK/Nodes/AKText.h>
#include <CZ/Marco/Nodes/MPopUpButton.h>
#include <CZ/Marco/Roles/MDialog.h>
#include <CZ/Marco/Extensions/MOutputManager.h>
#include <CZ/Core/CZTimer.h>
#include <memory>
#include <unordered_map>
#include <vector>

class DisplaysPane;

/// A display in the arrangement: selected when clicked, dragged to reorder.
class DisplayBox final : public CZ::AKRoundSolidColor
{
public:
    DisplayBox(DisplaysPane &pane, CZ::MOutputHead &head, CZ::AKNode *parent) noexcept;

    void setSelected(bool selected) noexcept;
    bool dragging() const noexcept { return m_dragging; }

    CZ::MOutputHead &head;
    SkScalar x {}, width {}; // Where it is in the arrangement (see DisplaysPane::layoutBoxes())
    CZ::AKText name { "", this };
    CZ::AKText size { "", this };

protected:
    void pointerButtonEvent(const CZ::CZPointerButtonEvent &e) override;
    void pointerMoveEvent(const CZ::CZPointerMoveEvent &e) override;
    void colorSchemeEvent(const CZ::CZColorSchemeEvent &e) override;

private:
    void updateColors() noexcept;
    DisplaysPane &m_pane;
    SkPoint m_grab {}; // Pointer, relative to the box (while dragging)
    bool m_selected { false }, m_dragging { false };
};

/// The arrangement's background (it lays the boxes out when resized).
class DisplayArrangement final : public CZ::AKRoundSolidColor
{
public:
    DisplayArrangement(DisplaysPane &pane, CZ::AKNode *parent) noexcept;

    static constexpr SkScalar Height { 200.f };
    static constexpr SkScalar Margin { 24.f };

protected:
    void layoutEvent(const CZ::CZLayoutEvent &e) override;
    void colorSchemeEvent(const CZ::CZColorSchemeEvent &e) override;

private:
    void updateColors() noexcept;
    DisplaysPane &m_pane;
};

/**
 * @brief Displays: how they're arranged, and the selected one's resolution, scale and rotation.
 *
 * Changes are kept pending until Apply. Then a dialog asks to keep them, and they're reverted
 * unless confirmed within ConfirmSeconds.
 *
 * The enabled displays are always arranged in a row, from (0, 0), each one starting right where
 * the previous one ends (dragging a box only changes the order).
 */
class DisplaysPane final : public Pane
{
public:
    DisplaysPane() noexcept;

    static constexpr int ConfirmSeconds { 20 };

    void select(CZ::MOutputHead *head) noexcept;

    /// Whether @p head is in use with the pending changes.
    bool pendingEnabled(CZ::MOutputHead *head) const noexcept;

    // From the boxes
    void boxDragged(DisplayBox &box, SkPoint pos) noexcept; // pos: relative to the arrangement
    void boxDropped(DisplayBox &box) noexcept;
    void layoutBoxes() noexcept;

    DisplayArrangement arrangement { *this, this };
    CZ::AKText unavailable { "Display settings aren't available: the compositor doesn't support output management.", this };

    CZ::AKFormSection settings { "", this };
    CZ::AKFormRow enabledRow { "Use this display", &settings.card };
    CZ::AKSwitch enabled { false, &enabledRow.trailing };
    CZ::AKFormRow resolutionRow { "Resolution", &settings.card };
    CZ::MPopUpButton resolution { {}, &resolutionRow.trailing };
    CZ::AKFormRow scaleRow { "Scale", &settings.card };
    CZ::MPopUpButton scale { {}, &scaleRow.trailing };
    CZ::AKFormRow rotationRow { "Rotation", &settings.card };
    CZ::MPopUpButton rotation { {}, &rotationRow.trailing };

    CZ::AKContainer buttons { YGFlexDirectionRow, false, this };
    CZ::AKButton apply { "Apply", &buttons };

protected:
    void colorSchemeEvent(const CZ::CZColorSchemeEvent &e) override;

private:
    // A display's setup, by name (its head may be gone and back meanwhile), to revert to
    struct Saved
    {
        std::string name;
        bool enabled;
        SkISize modeSize;
        CZ::Int32 refreshRate;
        SkIPoint pos;
        CZ::CZTransform transform;
        CZ::Float64 scale;
    };

    void reload() noexcept;
    void setEnabled(CZ::MOutputHead *head, bool enabled) noexcept;
    void normalize() noexcept;
    void rebuildBoxes() noexcept;
    void updateControls() noexcept;
    void updateColors() noexcept;
    bool dirty() const noexcept;
    SkISize logicalSize(CZ::MOutputHead *head) const noexcept;

    void applyPending() noexcept;
    void askToKeep() noexcept;
    void revert() noexcept;
    void showError(const std::string &title, const std::string &message) noexcept;
    void openDialog(std::unique_ptr<CZ::MDialog> dialog) noexcept;

    CZ::MOutputManager &m_manager;
    std::vector<std::unique_ptr<DisplayBox>> m_boxes;
    std::vector<CZ::MOutputHead*> m_order;    // The enabled ones (pending), left to right
    std::vector<CZ::MOutputHead*> m_disabled; // The others, shown dimmed after them
    std::unordered_map<CZ::MOutputHead*, CZ::MOutputHead::Config> m_pending;
    CZ::MOutputHead *m_selected {};
    std::vector<CZ::MOutputMode*> m_modeItems; // resolution's items
    std::vector<CZ::Float64> m_scaleItems;     // scale's items

    std::vector<Saved> m_previous;
    std::unique_ptr<CZ::MDialog> m_dialog;
    CZ::CZTimer m_countdown;
    int m_secondsLeft { 0 };
    bool m_revertPending { false }; // Cancelled (the outputs changed meanwhile): again once they're known
};

#endif // DISPLAYSPANE_H
