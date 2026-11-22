#ifndef SETTINGSSIDEBAR_H
#define SETTINGSSIDEBAR_H

#include <CZ/Marco/Nodes/MSidebar.h>
#include <CZ/AK/Nodes/AKSearchField.h>
#include <CZ/AK/Nodes/AKSidebarList.h>
#include <Sidebar/UserRow.h>

/**
 * @brief The sidebar of the Settings window.
 *
 * [window buttons]
 * [search field]
 * [user row, then the sections of settings (scrollable)]
 */
class SettingsSidebar final : public CZ::MSidebar
{
public:
    SettingsSidebar(CZ::AKNode *parent) noexcept;

    CZ::AKSearchField search { "Search", &content };
    CZ::AKSidebarList list { &content };
    UserRow user { &list };

private:
    void addSections() noexcept;
};

#endif // SETTINGSSIDEBAR_H
