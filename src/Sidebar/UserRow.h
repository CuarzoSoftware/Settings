#ifndef USERROW_H
#define USERROW_H

#include <CZ/AK/Nodes/AKContainer.h>
#include <CZ/AK/Nodes/AKAvatar.h>
#include <CZ/AK/Nodes/AKText.h>

/// The user running the app at the top of the sidebar: their picture, full name and login name.
class UserRow final : public CZ::AKContainer
{
public:
    UserRow(CZ::AKNode *parent) noexcept;

    static constexpr CZ::Int32 AvatarSize { 36 };

    CZ::AKAvatar avatar { AvatarSize, this };
    CZ::AKContainer texts { YGFlexDirectionColumn, false, this };
    CZ::AKText name { "", &texts };
    CZ::AKText detail { "", &texts };
};

#endif // USERROW_H
