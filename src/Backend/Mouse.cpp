#include <Backend/Mouse.h>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

static std::string Trim(const std::string &s) noexcept
{
    const auto b { s.find_first_not_of(" \t\r\n") };

    if (b == std::string::npos)
        return {};

    return s.substr(b, s.find_last_not_of(" \t\r\n") - b + 1);
}

std::filesystem::path MouseSettings::FilePath() noexcept
{
    std::filesystem::path dir;

    if (const char *config { getenv("XDG_CONFIG_HOME") }; config && config[0] == '/')
        dir = config;
    else if (const char *home { getenv("HOME") })
        dir = std::filesystem::path(home) / ".config";
    else
        return {};

    return dir / "cz-crystals" / "mouse.conf";
}

MouseSettings MouseSettings::Load() noexcept
{
    MouseSettings s;
    std::ifstream file { FilePath() };
    std::string line;

    while (std::getline(file, line))
    {
        const auto eq { line.find('=') };

        if (line.empty() || line[0] == '#' || line[0] == '[' || eq == std::string::npos)
            continue;

        const std::string key { Trim(line.substr(0, eq)) }, value { Trim(line.substr(eq + 1)) };

        if (key == "TrackingSpeed")
            s.trackingSpeed = std::clamp(std::atof(value.c_str()), -1., 1.);
        else if (key == "NaturalScrolling")
            s.naturalScrolling = value == "true" || value == "1";
        else if (key == "DoubleClickMs")
            s.doubleClickMs = std::clamp(std::atoi(value.c_str()), 100, 5000);
        else if (key == "ScrollSpeed")
            s.scrollSpeed = std::clamp(std::atof(value.c_str()), 0.25, 4.);
    }

    return s;
}

void MouseSettings::save() const noexcept
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

        // No spaces around '=': apps look for "DoubleClickMs=" (see CZMouseSettings)
        file << "# Written by System Settings (Mouse), applied by Crystals and the apps\n"
             << "[Mouse]\n"
             << "TrackingSpeed=" << trackingSpeed << "\n"
             << "NaturalScrolling=" << (naturalScrolling ? "true" : "false") << "\n"
             << "DoubleClickMs=" << doubleClickMs << "\n"
             << "ScrollSpeed=" << scrollSpeed << "\n";
    }

    std::filesystem::rename(tmp, path, ec);
}

void MouseSettings::exportDoubleClick() const noexcept
{
    const std::string ms { std::to_string(doubleClickMs) };

    // Forked twice: nothing to reap later
    const pid_t pid { fork() };

    if (pid == 0)
    {
        if (fork() == 0)
        {
            execlp("gsettings", "gsettings", "set", "org.gnome.desktop.peripherals.mouse", "double-click", ms.c_str(), nullptr);
            _exit(127);
        }

        _exit(0);
    }

    if (pid > 0)
        waitpid(pid, nullptr, 0);
}
