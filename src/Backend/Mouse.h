#ifndef MOUSE_H
#define MOUSE_H

#include <filesystem>

/**
 * @brief The mouse settings.
 *
 * Stored in `$XDG_CONFIG_HOME/cz-crystals/mouse.conf` (`~/.config/cz-crystals/mouse.conf` by
 * default). Crystals watches it and applies the tracking and scrolling ones right away (see its
 * MouseSettings, keep both in sync); apps read the double click time (see CZMouseSettings and
 * exportDoubleClick()).
 */
struct MouseSettings
{
    double trackingSpeed { 0. };    ///< -1: slowest … 1: fastest
    bool naturalScrolling { true };
    int doubleClickMs { 400 };      ///< Longest time between the two presses of a double click
    double scrollSpeed { 1. };      ///< What the wheel scrolls is multiplied by it (0.25 … 4)

    static std::filesystem::path FilePath() noexcept;
    static MouseSettings Load() noexcept;
    void save() const noexcept;

    /// Tells the other toolkits the double click time: GTK apps (Firefox and Chromium ones too)
    /// follow GNOME's `org.gnome.desktop.peripherals.mouse double-click` setting. Qt apps read
    /// the file (see cz-qtcuarzo), like Cuarzo's.
    void exportDoubleClick() const noexcept;
};

#endif // MOUSE_H
