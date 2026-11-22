#ifndef PANE_H
#define PANE_H

#include <CZ/AK/Nodes/AKContainer.h>
#include <string>

/**
 * @brief A page of settings, shown next to the sidebar (see SettingsWindow).
 *
 * A column of sections. The window shows its title in the toolbar and scrolls it.
 */
class Pane : public CZ::AKContainer
{
public:
    Pane(const std::string &id, const std::string &title) noexcept :
        CZ::AKContainer(YGFlexDirectionColumn, false),
        id(id), title(title)
    {
        layout().setGap(YGGutterRow, 22.f);
        layout().setWidthPercent(100.f);
    }

    /// Shown in the window / hidden (e.g. to refresh what it shows only meanwhile).
    virtual void paneShown() noexcept {}
    virtual void paneHidden() noexcept {}

    const std::string id, title;
};

#endif // PANE_H
