#include <Backend/AlertSettings.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <unistd.h>

static constexpr const char *SoundThemeDir { "/usr/share/sounds/freedesktop/stereo/" };

static std::string Trim(std::string s) noexcept
{
    const auto b { s.find_first_not_of(" \t\r\n") };

    if (b == std::string::npos)
        return {};

    return s.substr(b, s.find_last_not_of(" \t\r\n") - b + 1);
}

std::filesystem::path AlertSettings::FilePath() noexcept
{
    std::filesystem::path dir;

    if (const char *config { getenv("XDG_CONFIG_HOME") }; config && config[0] == '/')
        dir = config;
    else if (const char *home { getenv("HOME") })
        dir = std::filesystem::path(home) / ".config";
    else
        return {};

    return dir / "cz-crystals" / "sound.conf";
}

AlertSettings AlertSettings::Load() noexcept
{
    AlertSettings s;
    std::ifstream file { FilePath() };
    std::string line;

    const auto boolean = [](const std::string &v) { return v == "true" || v == "1" || v == "yes"; };

    while (std::getline(file, line))
    {
        const auto eq { line.find('=') };

        if (line.empty() || line[0] == '#' || line[0] == '[' || eq == std::string::npos)
            continue;

        const std::string key { Trim(line.substr(0, eq)) }, value { Trim(line.substr(eq + 1)) };

        if (key == "AlertSound")
            s.sound = value;
        else if (key == "AlertDevice")
            s.device = value;
        else if (key == "AlertVolume")
            s.volume = std::clamp(float(std::atof(value.c_str())), 0.f, 1.f);
        else if (key == "StartupSound")
            s.startupSound = boolean(value);
        else if (key == "InterfaceSounds")
            s.interfaceSounds = boolean(value);
        else if (key == "VolumeFeedback")
            s.volumeFeedback = boolean(value);
    }

    return s;
}

void AlertSettings::save() const noexcept
{
    const auto path { FilePath() };

    if (path.empty())
        return;

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    // Written aside and renamed: Crystals never reads half of it
    const auto tmp { std::filesystem::path(path).concat(".tmp") };

    {
        std::ofstream file { tmp, std::ios::trunc };

        if (!file)
            return;

        char volumeStr[16];
        snprintf(volumeStr, sizeof(volumeStr), "%.2f", double(volume));

        file << "# Written by System Settings (Sound), read by Crystals\n"
             << "[Sound]\n"
             << "AlertSound=" << sound << "\n"
             << "AlertDevice=" << device << "\n"
             << "AlertVolume=" << volumeStr << "\n"
             << "StartupSound=" << (startupSound ? "true" : "false") << "\n"
             << "InterfaceSounds=" << (interfaceSounds ? "true" : "false") << "\n"
             << "VolumeFeedback=" << (volumeFeedback ? "true" : "false") << "\n";
    }

    std::filesystem::rename(tmp, path, ec);
}

const std::vector<AlertSettings::Sound> &AlertSettings::Sounds() noexcept
{
    static const std::vector<Sound> sounds { []{
        const std::pair<const char*, const char*> known[] {
            { "bell",                "Bell" },
            { "complete",            "Complete" },
            { "dialog-information",  "Information" },
            { "dialog-warning",      "Warning" },
            { "dialog-error",        "Error" },
            { "message",             "Message" },
            { "message-new-instant", "Instant Message" },
            { "window-attention",    "Attention" },
            { "window-question",     "Question" },
            { "alarm-clock-elapsed", "Alarm" },
        };

        std::vector<Sound> list;

        for (const auto &[id, name] : known)
        {
            const std::string path { std::string(SoundThemeDir) + id + ".oga" };

            if (access(path.c_str(), R_OK) == 0)
                list.push_back({ id, name, path });
        }

        return list;
    }() };

    return sounds;
}

std::string AlertSettings::SoundPath(const std::string &id) noexcept
{
    for (const auto &s : Sounds())
        if (s.id == id)
            return s.path;

    return {};
}
