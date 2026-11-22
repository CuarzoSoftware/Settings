#include <Sidebar/SettingsSidebar.h>
#include <CZ/AK/AKTheme.h>

using namespace CZ;

SettingsSidebar::SettingsSidebar(AKNode *parent) noexcept : MSidebar(parent)
{
    search.layout().setMargin(YGEdgeHorizontal, AKSidebarList::Padding);
    search.layout().setMargin(YGEdgeBottom, 10.f);

    list.layout().setFlex(1.f);

    addSections();

}

void SettingsSidebar::addSections() noexcept
{
    struct Section { const char *id, *text, *icon; CZAdaptiveColor color; };

    // Grouped like macOS's System Settings. nullptr: a new section.
    static const Section sections[]
    {
        { "wifi",          "Wi-Fi",               "wifi",              AKTheme::SystemBlue },
        { "bluetooth",     "Bluetooth",           "bluetooth",         AKTheme::SystemBlue },
        { "network",       "Network",             "lan",               AKTheme::SystemBlue },
        { "battery",       "Battery",             "battery_full",      AKTheme::SystemGreen },
        { nullptr, nullptr, nullptr, {} },
        { "general",       "General",             "settings",          AKTheme::SystemGray },
        { "accessibility", "Accessibility",       "accessibility_new", AKTheme::SystemBlue },
        { "appearance",    "Appearance",          "contrast",          CZAdaptiveColor(0xFF1C1C1E, 0xFF48484A) },
        { "controlcenter", "Control Center",      "toggle_on",         AKTheme::SystemGray },
        { "desktop",       "Desktop & Dock",      "desktop_mac",       CZAdaptiveColor(0xFF1C1C1E, 0xFF48484A) },
        { "displays",      "Displays",            "monitor",           AKTheme::SystemBlue },
        { "screensaver",   "Screen Saver",        "screenshot_monitor",AKTheme::SystemTeal },
        { "wallpaper",     "Wallpaper",           "wallpaper",         AKTheme::SystemCyan },
        { nullptr, nullptr, nullptr, {} },
        { "notifications", "Notifications",       "notifications",     AKTheme::SystemRed },
        { "sound",         "Sound",               "volume_up",         AKTheme::SystemPink },
        { "focus",         "Focus",               "bedtime",           AKTheme::SystemIndigo },
        { nullptr, nullptr, nullptr, {} },
        { "lockscreen",    "Lock Screen",         "lock",              CZAdaptiveColor(0xFF1C1C1E, 0xFF48484A) },
        { "privacy",       "Privacy & Security",  "back_hand",         AKTheme::SystemBlue },
        { "users",         "Users & Groups",      "group",             AKTheme::SystemBlue },
        { nullptr, nullptr, nullptr, {} },
        { "accounts",      "Internet Accounts",   "alternate_email",   AKTheme::SystemBlue },
        { "passwords",     "Passwords",           "key",               AKTheme::SystemGray },
        { nullptr, nullptr, nullptr, {} },
        { "keyboard",      "Keyboard",            "keyboard",          AKTheme::SystemGray },
        { "mouse",         "Mouse",               "mouse",             AKTheme::SystemGray },
        { "trackpad",      "Trackpad",            "touch_app",         AKTheme::SystemGray },
        { "printers",      "Printers & Scanners", "print",             AKTheme::SystemGray },
    };

    for (const Section &s : sections)
    {
        if (!s.id)
            list.addSection();
        else
            list.addItem(s.text, s.icon, s.color, s.id);
    }
}
