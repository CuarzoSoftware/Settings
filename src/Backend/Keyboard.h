#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <CZ/Core/CZObject.h>
#include <CZ/Core/CZSignal.h>
#include <CZ/Core/CZBusWorker.h>
#include <CZ/Core/CZBus.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

struct sd_bus_slot;

/// A keyboard layout (XKB layout and variant), e.g. { "latam", "" } or { "us", "intl" }.
struct InputSource
{
    std::string layout, variant;
    bool operator==(const InputSource &other) const noexcept = default;
};

/**
 * @brief The keyboard settings applied by Crystals.
 *
 * Stored in `$XDG_CONFIG_HOME/cz-crystals/keyboard.conf` (`~/.config/cz-crystals/keyboard.conf` by
 * default). Crystals watches it and applies changes right away (see its KeyboardSettings, keep
 * both in sync). Without it, the system's layout (localectl) is used.
 */
struct KeyboardSettings
{
    CZ::Int32 repeatRate { 32 };    ///< Repeats per second, 0: no repeat
    CZ::Int32 repeatDelay { 500 };  ///< Milliseconds before repeating
    std::vector<InputSource> sources; ///< At least one. Several: Super+Space switches between them
    CZ::Int32 backlightTimeout { 0 }; ///< Seconds without input before the backlight turns off, 0: never

    static std::filesystem::path FilePath() noexcept;
    static KeyboardSettings Load() noexcept;
    void save() const noexcept;

    /// The system's layouts (`/etc/X11/xorg.conf.d/00-keyboard.conf`, set with localectl), or "us".
    static std::vector<InputSource> SystemSources() noexcept;
};

/// The layouts XKB knows (from its rules), with their names.
struct XkbCatalog
{
    struct Entry
    {
        InputSource source;
        std::string name; ///< e.g. "Spanish (Latin American)", "English (US, intl., with dead keys)"
    };

    /// Every layout and variant, by name. Loaded on first use.
    static const std::vector<Entry> &Entries() noexcept;

    /// The name of @p source (its ids if unknown).
    static std::string Name(const InputSource &source) noexcept;
};

/**
 * @brief The keyboard backlight, through UPower (org.freedesktop.UPower.KbdBacklight).
 *
 * available() is false without one. Changes made elsewhere (e.g. a brightness key) emit onChanged.
 */
class KeyboardBacklight final : public CZ::CZObject
{
public:
    KeyboardBacklight() noexcept;
    ~KeyboardBacklight() noexcept;

    bool available() const noexcept { return m_max > 0; }
    CZ::Int32 max() const noexcept { return m_max; }
    CZ::Int32 brightness() const noexcept { return m_brightness; }

    /// Sets it (clamped to [0, max()]).
    void setBrightness(CZ::Int32 brightness) noexcept;

    CZ::CZSignal<> onChanged;

private:
    CZ::CZBusWorker m_worker { CZ::CZBusWorker::System };
    std::shared_ptr<CZ::CZBus> m_system; // Kept: the signal match lives while it's held
    sd_bus_slot *m_match {};
    CZ::Int32 m_max { 0 }, m_brightness { 0 };
};

#endif // KEYBOARD_H
