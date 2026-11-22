#ifndef PLACEHOLDERPANE_H
#define PLACEHOLDERPANE_H

#include <Panes/Pane.h>
#include <CZ/AK/Nodes/AKIconTile.h>
#include <CZ/AK/Nodes/AKText.h>

/// A section without settings yet: its icon and name.
class PlaceholderPane final : public Pane
{
public:
    PlaceholderPane(const std::string &id, const std::string &title, const std::string &icon, CZ::CZAdaptiveColor color) noexcept;

    CZ::AKIconTile tile;
    CZ::AKText name { "", this };
    CZ::AKText note { "Coming soon.", this };
};

#endif // PLACEHOLDERPANE_H
