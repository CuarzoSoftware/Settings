#ifndef SETTINGSSIDEBAR_H
#define SETTINGSSIDEBAR_H

#include <CZ/Marco/Nodes/MSidebar.h>
#include <CZ/AK/Nodes/AKSearchField.h>
#include <CZ/AK/Nodes/AKSidebarList.h>
#include <CZ/AK/Effects/AKEdgeShadow.h>
#include <Sidebar/UserRow.h>

/**
 * @brief The sidebar of the Settings window.
 *
 * [window buttons]
 * [search field]           (a shadow below it while the list is scrolled)
 * [user row, then the sections of settings (scrollable)]
 */
class SettingsSidebar final : public CZ::MSidebar
{
public:
    SettingsSidebar(CZ::AKNode *parent) noexcept;

    CZ::AKSearchField search { "Search", &content };
    CZ::AKSidebarList list { &content };
    UserRow user { &list };

    // Over the list (added last), as tall as the search field: its shadow falls on the list while
    // it's scrolled under it
    CZ::AKContainer searchShadowArea { YGFlexDirectionColumn, false, &content };
    CZ::AKEdgeShadow searchShadow { CZ::CZEdgeBottom, &searchShadowArea };

private:
    void addSections() noexcept;
};

#endif // SETTINGSSIDEBAR_H
