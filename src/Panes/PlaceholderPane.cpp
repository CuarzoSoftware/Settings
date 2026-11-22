#include <Panes/PlaceholderPane.h>
#include <CZ/AK/AKTheme.h>

using namespace CZ;

PlaceholderPane::PlaceholderPane(const std::string &id, const std::string &title, const std::string &icon, CZAdaptiveColor color) noexcept :
    Pane(id, title),
    tile(icon, color, 64, this)
{
    layout().setAlignItems(YGAlignCenter);
    layout().setGap(YGGutterRow, 10.f);
    layout().setPadding(YGEdgeTop, 60.f);

    name.setText(title);
    name.setFontSize(20.f);
    name.setFontStyle(SkFontStyle(SkFontStyle::kBold_Weight, SkFontStyle::kNormal_Width, SkFontStyle::kUpright_Slant));
    name.setTextColor(AKTheme::Label);

    note.setFontSize(13.f);
    note.setTextColor(AKTheme::SecondaryLabel);
}
