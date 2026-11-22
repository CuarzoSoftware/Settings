#include <Backend/Keyboard.h>
#include <CZ/Core/CZLog.h>
#include <systemd/sd-bus.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

using namespace CZ;

static constexpr const char *XkbRules { "/usr/share/X11/xkb/rules/evdev.xml" };
static constexpr const char *SystemKeyboardConf { "/etc/X11/xorg.conf.d/00-keyboard.conf" };
static constexpr const char *UPower { "org.freedesktop.UPower" };
static constexpr const char *KbdPath { "/org/freedesktop/UPower/KbdBacklight" };
static constexpr const char *KbdIface { "org.freedesktop.UPower.KbdBacklight" };

static std::string Trim(const std::string &s) noexcept
{
    const auto b { s.find_first_not_of(" \t\r\n") };

    if (b == std::string::npos)
        return {};

    return s.substr(b, s.find_last_not_of(" \t\r\n") - b + 1);
}

static std::vector<std::string> Split(const std::string &s, char sep) noexcept
{
    std::vector<std::string> out;
    std::string item;
    std::istringstream in { s };

    while (std::getline(in, item, sep))
        out.push_back(Trim(item));

    if (!s.empty() && s.back() == sep)
        out.emplace_back();

    return out;
}

/* ------------------------------------- KeyboardSettings ------------------------------------- */

std::filesystem::path KeyboardSettings::FilePath() noexcept
{
    std::filesystem::path dir;

    if (const char *config { getenv("XDG_CONFIG_HOME") }; config && config[0] == '/')
        dir = config;
    else if (const char *home { getenv("HOME") })
        dir = std::filesystem::path(home) / ".config";
    else
        return {};

    return dir / "cz-crystals" / "keyboard.conf";
}

std::vector<InputSource> KeyboardSettings::SystemSources() noexcept
{
    // Option "XkbLayout" "latam,us" / Option "XkbVariant" ",intl"
    std::ifstream file { SystemKeyboardConf };
    std::string line, layouts, variants;

    const auto value = [](const std::string &line) -> std::string {
        const auto last { line.rfind('"') };
        const auto first { last == std::string::npos || last == 0 ? std::string::npos : line.rfind('"', last - 1) };
        return first == std::string::npos ? std::string() : line.substr(first + 1, last - first - 1);
    };

    while (std::getline(file, line))
    {
        if (line.find("\"XkbLayout\"") != std::string::npos)
            layouts = value(line);
        else if (line.find("\"XkbVariant\"") != std::string::npos)
            variants = value(line);
    }

    std::vector<InputSource> sources;
    const auto l { Split(layouts, ',') }, v { Split(variants, ',') };

    for (size_t i = 0; i < l.size(); i++)
        if (!l[i].empty())
            sources.push_back({ l[i], i < v.size() ? v[i] : "" });

    if (sources.empty())
        sources.push_back({ "us", "" });

    return sources;
}

KeyboardSettings KeyboardSettings::Load() noexcept
{
    KeyboardSettings s;
    std::ifstream file { FilePath() };
    std::string line, layouts, variants;
    bool hasLayouts { false };

    while (std::getline(file, line))
    {
        const auto eq { line.find('=') };

        if (line.empty() || line[0] == '#' || line[0] == '[' || eq == std::string::npos)
            continue;

        const std::string key { Trim(line.substr(0, eq)) }, value { Trim(line.substr(eq + 1)) };

        if (key == "RepeatRate")
            s.repeatRate = std::clamp(std::atoi(value.c_str()), 0, 100);
        else if (key == "RepeatDelay")
            s.repeatDelay = std::clamp(std::atoi(value.c_str()), 100, 5000);
        else if (key == "Layouts")
        {
            layouts = value;
            hasLayouts = true;
        }
        else if (key == "Variants")
            variants = value;
        else if (key == "BacklightTimeout")
            s.backlightTimeout = std::max(0, std::atoi(value.c_str()));
    }

    if (hasLayouts)
    {
        const auto l { Split(layouts, ',') }, v { Split(variants, ',') };

        for (size_t i = 0; i < l.size(); i++)
            if (!l[i].empty())
                s.sources.push_back({ l[i], i < v.size() ? v[i] : "" });
    }

    if (s.sources.empty())
        s.sources = SystemSources();

    return s;
}

void KeyboardSettings::save() const noexcept
{
    const auto path { FilePath() };

    if (path.empty())
        return;

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::string layouts, variants;

    for (size_t i = 0; i < sources.size(); i++)
    {
        layouts += (i ? "," : "") + sources[i].layout;
        variants += (i ? "," : "") + sources[i].variant;
    }

    // Written aside and renamed: Crystals (watching it) never reads half of it
    const auto tmp { std::filesystem::path(path).concat(".tmp") };

    {
        std::ofstream file { tmp, std::ios::trunc };

        if (!file)
            return;

        file << "# Written by System Settings (Keyboard), applied by Crystals\n"
             << "[Keyboard]\n"
             << "RepeatRate=" << repeatRate << "\n"
             << "RepeatDelay=" << repeatDelay << "\n"
             << "Layouts=" << layouts << "\n"
             << "Variants=" << variants << "\n"
             << "BacklightTimeout=" << backlightTimeout << "\n";
    }

    std::filesystem::rename(tmp, path, ec);
}

/* ---------------------------------------- XkbCatalog ---------------------------------------- */

static std::string XmlText(const std::string &s) noexcept
{
    std::string out;

    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == '&')
        {
            static const std::pair<const char*, char> entities[] {
                { "&amp;", '&' }, { "&lt;", '<' }, { "&gt;", '>' }, { "&quot;", '"' }, { "&apos;", '\'' } };
            bool found { false };

            for (const auto &[entity, c] : entities)
                if (s.compare(i, strlen(entity), entity) == 0)
                {
                    out += c;
                    i += strlen(entity) - 1;
                    found = true;
                    break;
                }

            if (found)
                continue;
        }

        out += s[i];
    }

    return out;
}

// The text of the first <tag>…</tag> in [from, to), empty if none
static std::string Tag(const std::string &xml, const char *tag, size_t from, size_t to) noexcept
{
    const std::string open { std::string("<") + tag + ">" }, close { std::string("</") + tag + ">" };
    const auto b { xml.find(open, from) };

    if (b == std::string::npos || b >= to)
        return {};

    const auto e { xml.find(close, b) };

    if (e == std::string::npos || e > to)
        return {};

    return XmlText(Trim(xml.substr(b + open.size(), e - b - open.size())));
}

const std::vector<XkbCatalog::Entry> &XkbCatalog::Entries() noexcept
{
    static const std::vector<Entry> entries { []{
        std::vector<Entry> list;
        std::ifstream file { XkbRules };
        std::stringstream buf;
        buf << file.rdbuf();
        const std::string xml { buf.str() };

        // Only the layouts (not the models or options)
        const auto listBegin { xml.find("<layoutList>") };
        const auto listEnd { xml.find("</layoutList>") };

        if (listBegin == std::string::npos || listEnd == std::string::npos)
            return list;

        size_t pos { listBegin };

        while ((pos = xml.find("<layout>", pos)) != std::string::npos && pos < listEnd)
        {
            const auto end { xml.find("</layout>", pos) };

            if (end == std::string::npos)
                break;

            // The layout's own configItem comes before its variants
            const auto variantsAt { std::min(xml.find("<variantList>", pos), end) };
            const std::string name { Tag(xml, "name", pos, variantsAt) };
            const std::string description { Tag(xml, "description", pos, variantsAt) };

            if (!name.empty())
                list.push_back({ { name, "" }, description.empty() ? name : description });

            size_t v { variantsAt };

            while ((v = xml.find("<variant>", v)) != std::string::npos && v < end)
            {
                const auto vEnd { xml.find("</variant>", v) };

                if (vEnd == std::string::npos || vEnd > end)
                    break;

                const std::string vName { Tag(xml, "name", v, vEnd) };
                const std::string vDescription { Tag(xml, "description", v, vEnd) };

                if (!name.empty() && !vName.empty())
                    list.push_back({ { name, vName }, vDescription.empty() ? name + " (" + vName + ")" : vDescription });

                v = vEnd;
            }

            pos = end;
        }

        std::sort(list.begin(), list.end(), [](const Entry &a, const Entry &b){ return a.name < b.name; });
        return list;
    }() };

    return entries;
}

std::string XkbCatalog::Name(const InputSource &source) noexcept
{
    for (const auto &e : Entries())
        if (e.source == source)
            return e.name;

    return source.variant.empty() ? source.layout : source.layout + " (" + source.variant + ")";
}

/* ------------------------------------- KeyboardBacklight ------------------------------------- */

KeyboardBacklight::KeyboardBacklight() noexcept
{
    m_worker.run([this](sd_bus *bus) -> std::function<void()> {
        Int32 max { 0 }, brightness { 0 };

        if (bus)
        {
            sd_bus_message *reply {};

            if (sd_bus_call_method(bus, UPower, KbdPath, KbdIface, "GetMaxBrightness", nullptr, &reply, "") >= 0)
            {
                sd_bus_message_read(reply, "i", &max);
                reply = sd_bus_message_unref(reply);
            }

            if (max > 0 && sd_bus_call_method(bus, UPower, KbdPath, KbdIface, "GetBrightness", nullptr, &reply, "") >= 0)
            {
                sd_bus_message_read(reply, "i", &brightness);
                sd_bus_message_unref(reply);
            }
        }

        return [this, max, brightness]{
            m_max = max;
            m_brightness = brightness;
            onChanged.notify();
        };
    });

    // Changed elsewhere (e.g. a brightness key, or Crystals turning it off after inactivity)
    m_system = CZBus::GetOrMakeSystem();

    if (m_system)
    {
        sd_bus_match_signal_async(m_system->bus(), &m_match, UPower, KbdPath, KbdIface, "BrightnessChanged",
            [](sd_bus_message *m, void *data, sd_bus_error *) -> int {
                Int32 value { 0 };

                if (sd_bus_message_read(m, "i", &value) >= 0)
                {
                    auto *self { static_cast<KeyboardBacklight*>(data) };

                    if (self->m_brightness != value)
                    {
                        self->m_brightness = value;
                        self->onChanged.notify();
                    }
                }

                return 0;
            }, nullptr, this);

        sd_bus_flush(m_system->bus());
    }
}

KeyboardBacklight::~KeyboardBacklight() noexcept
{
    sd_bus_slot_unref(m_match);
}

void KeyboardBacklight::setBrightness(Int32 brightness) noexcept
{
    brightness = std::clamp(brightness, 0, m_max);

    if (!available() || brightness == m_brightness)
        return;

    m_brightness = brightness;

    m_worker.run([brightness](sd_bus *bus) -> std::function<void()> {
        if (bus && sd_bus_call_method(bus, UPower, KbdPath, KbdIface, "SetBrightness", nullptr, nullptr, "i", brightness) < 0)
            CZLog(CZWarning, CZLN, "Couldn't set the keyboard brightness (UPower)");

        return {};
    });
}
