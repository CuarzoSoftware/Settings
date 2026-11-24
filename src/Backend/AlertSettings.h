#ifndef ALERTSETTINGS_H
#define ALERTSETTINGS_H

#include <filesystem>
#include <string>
#include <vector>

/**
 * @brief The alert and feedback sounds' settings, used by Crystals (which plays them).
 *
 * Stored in `$XDG_CONFIG_HOME/cz-crystals/sound.conf` (`~/.config/cz-crystals/sound.conf` by
 * default), read by Crystals whenever it plays one (see its SoundSettings, keep both in sync).
 */
struct AlertSettings
{
    std::string sound { "bell" };   ///< A name from Sounds()
    std::string device;             ///< Output (name), empty: the selected (default) one
    float volume { 0.75f };         ///< 0..1
    bool startupSound { false };    ///< Played when Crystals starts
    bool interfaceSounds { true };  ///< Crystals' own sound effects
    bool volumeFeedback { true };   ///< Played when the volume keys change the volume

    static std::filesystem::path FilePath() noexcept;
    static AlertSettings Load() noexcept;
    void save() const noexcept;

    /// An alert sound: its name, as shown, and file.
    struct Sound { std::string id, name, path; };

    /// The alert sounds available (from the freedesktop sound theme).
    static const std::vector<Sound> &Sounds() noexcept;

    /// The file of @p id (empty if unknown).
    static std::string SoundPath(const std::string &id) noexcept;
};

#endif // ALERTSETTINGS_H
