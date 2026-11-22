#include <Sidebar/UserRow.h>
#include <CZ/AK/AKTheme.h>

using namespace CZ;

UserRow::UserRow(AKNode *parent) noexcept : AKContainer(YGFlexDirectionRow, false, parent)
{
    layout().setAlignItems(YGAlignCenter);
    layout().setGap(YGGutterColumn, 8.f);
    layout().setPadding(YGEdgeHorizontal, 4.f);
    layout().setPadding(YGEdgeVertical, 8.f);
    layout().setMargin(YGEdgeBottom, 6.f);
    layout().setFlexShrink(0.f);

    avatar.setPictureFromFile(AKAvatar::UserPicturePath());

    texts.layout().setFlexShrink(1.f);

    name.setText(AKAvatar::UserRealName());
    name.setFontSize(13.f);
    name.setFontStyle(SkFontStyle(SkFontStyle::kSemiBold_Weight, SkFontStyle::kNormal_Width, SkFontStyle::kUpright_Slant));
    name.setTextColor(AKTheme::Label);

    detail.setText(AKAvatar::UserLoginName());
    detail.setFontSize(11.f);
    detail.setTextColor(AKTheme::SecondaryLabel);
}
