#include <Backend/Trackpad.h>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <string>

static std::string Trim(const std::string &s) noexcept
{
    const auto b { s.find_first_not_of(" \t\r\n") };

    if (b == std::string::npos)
        return {};

    return s.substr(b, s.find_last_not_of(" \t\r\n") - b + 1);
}

std::filesystem::path TrackpadSettings::FilePath() noexcept
{
    std::filesystem::path dir;

    if (const char *config { getenv("XDG_CONFIG_HOME") }; config && config[0] == '/')
        dir = config;
    else if (const char *home { getenv("HOME") })
        dir = std::filesystem::path(home) / ".config";
    else
        return {};

    return dir / "cz-crystals" / "trackpad.conf";
}

TrackpadSettings TrackpadSettings::Load() noexcept
{
    TrackpadSettings s;
    std::ifstream file { FilePath() };
    std::string line;

    while (std::getline(file, line))
    {
        const auto eq { line.find('=') };

        if (line.empty() || line[0] == '#' || line[0] == '[' || eq == std::string::npos)
            continue;

        const std::string key { Trim(line.substr(0, eq)) }, value { Trim(line.substr(eq + 1)) };
        const bool on { value == "true" || value == "1" };

        if (key == "TrackingSpeed")
            s.trackingSpeed = std::clamp(std::atof(value.c_str()), -1., 1.);
        else if (key == "SecondaryClick")
            s.secondaryClick = value == "BottomRight" ? SecondaryClick::BottomRight : SecondaryClick::TwoFingers;
        else if (key == "TapToClick")
            s.tapToClick = on;
        else if (key == "NaturalScrolling")
            s.naturalScrolling = on;
        else if (key == "WorkspaceSwipe")
            s.workspaceSwipe = on;
        else if (key == "Expose")
            s.expose = on;
        else if (key == "ShowDesktop")
            s.showDesktop = on;
    }

    return s;
}

void TrackpadSettings::save() const noexcept
{
    const auto path { FilePath() };

    if (path.empty())
        return;

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    // Written aside and renamed: Crystals (watching it) never reads half of it
    const auto tmp { std::filesystem::path(path).concat(".tmp") };

    {
        std::ofstream file { tmp, std::ios::trunc };

        if (!file)
            return;

        const auto flag = [](bool on) { return on ? "true" : "false"; };

        file << "# Written by System Settings (Trackpad), applied by Crystals\n"
             << "[Trackpad]\n"
             << "TrackingSpeed=" << trackingSpeed << "\n"
             << "SecondaryClick=" << (secondaryClick == SecondaryClick::BottomRight ? "BottomRight" : "TwoFingers") << "\n"
             << "TapToClick=" << flag(tapToClick) << "\n"
             << "NaturalScrolling=" << flag(naturalScrolling) << "\n"
             << "WorkspaceSwipe=" << flag(workspaceSwipe) << "\n"
             << "Expose=" << flag(expose) << "\n"
             << "ShowDesktop=" << flag(showDesktop) << "\n";
    }

    std::filesystem::rename(tmp, path, ec);
}
