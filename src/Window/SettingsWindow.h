#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <CZ/Marco/Roles/MToplevel.h>
#include <Sidebar/SettingsSidebar.h>
#include <Panes/Pane.h>
#include <CZ/AK/Nodes/AKSolidColor.h>
#include <CZ/AK/Nodes/AKScroll.h>
#include <CZ/AK/Nodes/AKIconButton.h>
#include <CZ/AK/Nodes/AKText.h>
#include <CZ/AK/Effects/AKEdgeShadow.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief The System Settings window, like macOS's.
 *
 * [sidebar (vibrancy, window buttons, the sections)][toolbar (back, forward, title) / the selected pane]
 *
 * Selecting a section shows its pane (created when first shown). Back and forward go through the
 * panes shown before, like a browser.
 */
class SettingsWindow final : public CZ::MToplevel
{
public:
    SettingsWindow() noexcept;

    static constexpr CZ::Int32 DefaultWidth    { 840 };
    static constexpr CZ::Int32 DefaultHeight   { 700 };
    static constexpr CZ::Int32 SidebarWidth    { 270 }; // Default (the user can resize it)
    static constexpr CZ::Int32 MinPaneWidth    { 480 };
    static constexpr CZ::Int32 MinHeight       { 400 };
    static constexpr SkScalar  PanePadding     { 20.f };

    /// Shows the pane of the section @p id (selecting it in the sidebar).
    void show(const std::string &id) noexcept;

    void goBack() noexcept;
    void goForward() noexcept;

    SettingsSidebar sidebar { this };

    // The selected pane: a toolbar row the window is dragged by, then its content (scrolled).
    struct PaneArea : public CZ::AKSolidColor
    {
        PaneArea(CZ::AKNode *parent) noexcept;
        CZ::AKContainer toolbar { YGFlexDirectionRow, false, this };
        CZ::AKIconButton back { "chevron_left", 26, &toolbar };
        CZ::AKIconButton forward { "chevron_right", 26, &toolbar };
        CZ::AKText title { "", &toolbar };
        CZ::AKScroll scroll { this };
        CZ::AKEdgeShadow separator { CZ::CZEdgeLeft, this }; // Over the sidebar's trailing edge

        // Over the pane (added last), as tall as the toolbar: its shadow falls on the pane while
        // it's scrolled under it
        CZ::AKContainer toolbarShadowArea { YGFlexDirectionColumn, false, this };
        CZ::AKEdgeShadow toolbarShadow { CZ::CZEdgeBottom, &toolbarShadowArea };
    } paneArea { this };

private:
    Pane *pane(const std::string &id) noexcept;
    void present(const std::string &id) noexcept;
    void updateNavigation() noexcept;

    std::unordered_map<std::string, std::unique_ptr<Pane>> m_panes;
    Pane *m_current { nullptr };
    std::vector<std::string> m_backStack, m_forwardStack;
    bool m_navigating { false }; // Going back/forward: not recorded
};

#endif // SETTINGSWINDOW_H
