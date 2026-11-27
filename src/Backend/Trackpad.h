#ifndef TRACKPAD_H
#define TRACKPAD_H

#include <filesystem>

/**
 * @brief The trackpad settings applied by Crystals.
 *
 * Stored in `$XDG_CONFIG_HOME/cz-crystals/trackpad.conf` (`~/.config/cz-crystals/trackpad.conf` by
 * default). Crystals watches it and applies changes right away (see its TrackpadSettings, keep
 * both in sync).
 */
struct TrackpadSettings
{
    enum class SecondaryClick
    {
        TwoFingers,  ///< Click or tap with two fingers
        BottomRight, ///< Click in the bottom right corner
    };

    double trackingSpeed { 0. };        ///< -1: slowest … 1: fastest
    SecondaryClick secondaryClick { SecondaryClick::TwoFingers };
    bool tapToClick { false };
    bool naturalScrolling { true };

    // The three finger swipes
    bool workspaceSwipe { true };       ///< Sideways: the next / previous desktop
    bool expose { true };               ///< Up: Exposé
    bool showDesktop { true };          ///< Down: the windows move aside

    static std::filesystem::path FilePath() noexcept;
    static TrackpadSettings Load() noexcept;
    void save() const noexcept;
};

#endif // TRACKPAD_H
