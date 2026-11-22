#ifndef SOUND_H
#define SOUND_H

#include <CZ/Core/CZObject.h>
#include <CZ/Core/CZSignal.h>
#include <CZ/Core/CZBusWorker.h>
#include <CZ/Core/CZTimer.h>
#include <CZ/Core/CZEventSource.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <sys/types.h>

/// An audio output (sink) or input (source).
struct AudioDevice
{
    std::string name;        ///< PipeWire/PulseAudio name (what pactl takes)
    std::string displayName; ///< Shown: e.g. "Speaker" for built-in ones, the product otherwise
    std::string type;        ///< "Built-in", "USB", "Bluetooth", "HDMI" or "Virtual"
    float volume { 0.f };    ///< 0..1 (average of its channels; can be more if boosted elsewhere)
    bool muted { false };
};

/// The audio devices as shown by the Sound pane.
struct SoundState
{
    bool loaded { false };    ///< Read at least once
    bool available { false }; ///< pactl works (PipeWire or PulseAudio running)
    std::vector<AudioDevice> outputs, inputs;
    std::string defaultOutput, defaultInput; ///< Names

    const AudioDevice *output(const std::string &name) const noexcept;
    const AudioDevice *input(const std::string &name) const noexcept;
};

/**
 * @brief The alert and feedback sounds' settings, used by Crystals (which plays them).
 *
 * Stored in `$XDG_CONFIG_HOME/cz-crystals/sound.conf` (`~/.config/cz-crystals/sound.conf` by
 * default), read by Crystals whenever it plays one (see its SoundSettings, keep both in sync).
 */
struct AlertSettings
{
    std::string sound { "bell" };   ///< A name from Sound::AlertSounds()
    std::string device;             ///< Output (name), empty: the selected (default) one
    float volume { 0.75f };         ///< 0..1
    bool startupSound { false };    ///< Played when Crystals starts
    bool interfaceSounds { true };  ///< Crystals' own sound effects
    bool volumeFeedback { true };   ///< Played when the volume keys change the volume

    static std::filesystem::path FilePath() noexcept;
    static AlertSettings Load() noexcept;
    void save() const noexcept;
};

/**
 * @brief The audio devices (through `pactl`, so PipeWire or PulseAudio): listing them, choosing the
 *        default ones, their volume, and the input level.
 *
 * Commands run off the UI thread; onChanged tells when state() changed (watched with
 * `pactl subscribe`, so changes from anywhere show up).
 */
class Sound final : public CZ::CZObject
{
public:
    Sound() noexcept;
    ~Sound() noexcept;

    const SoundState &state() const noexcept { return m_state; }

    /// Makes @p name the default output (or input).
    void setDefault(bool input, const std::string &name) noexcept;

    /// Sets the volume (coalesced: the last one wins while a previous one runs).
    void setVolume(bool input, const std::string &name, float volume) noexcept;

    void setMuted(bool input, const std::string &name, bool muted) noexcept;

    /**
     * @brief Measures the level of the input @p name (onInputLevel) until stopInputLevel().
     *
     * Records from it (like any app doing so, it shows as in use meanwhile).
     */
    void startInputLevel(const std::string &name) noexcept;
    void stopInputLevel() noexcept;

    /// An alert sound: its name, as shown, and file.
    struct AlertSound { std::string id, name, path; };

    /// The alert sounds available (from the freedesktop sound theme).
    static const std::vector<AlertSound> &AlertSounds() noexcept;

    /// The file of @p id (empty if unknown).
    static std::string AlertSoundPath(const std::string &id) noexcept;

    /// Plays @p path through @p device (empty: the default output) at @p volume.
    void play(const std::string &path, const std::string &device, float volume) noexcept;

    CZ::CZSignal<> onChanged;
    CZ::CZSignal<float> onInputLevel; ///< 0..1, a few times per second while measuring

private:
    void refresh() noexcept;
    void applyVolume() noexcept;
    void startMonitor() noexcept;
    void stopMonitor() noexcept;
    void handleLevelData(int fd) noexcept;

    CZ::CZBusWorker m_worker { CZ::CZBusWorker::User };
    SoundState m_state;
    bool m_busy { false }, m_again { false };
    std::unique_ptr<CZ::CZTimer> m_refreshTimer, m_retryTimer;

    // Volume changes waiting to be applied (the last one per device)
    struct PendingVolume { bool input; std::string name; float volume; };
    std::vector<PendingVolume> m_pendingVolumes;
    bool m_volumeBusy { false };

    // pactl subscribe
    pid_t m_monitorPid { -1 };
    std::shared_ptr<CZ::CZEventSource> m_monitorSource;
    std::string m_monitorBuffer;

    // parec (input level)
    pid_t m_levelPid { -1 };
    std::shared_ptr<CZ::CZEventSource> m_levelSource;
    float m_level { 0.f };
};

#endif // SOUND_H
