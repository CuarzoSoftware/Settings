#include <Panes/SoundPane.h>
#include <CZ/AK/AKTheme.h>
#include <CZ/Core/Events/CZColorSchemeEvent.h>
#include <CZ/Core/Events/CZPointerButtonEvent.h>
#include <linux/input-event-codes.h>
#include <algorithm>

using namespace CZ;

static constexpr SkScalar TableRowHeight { 28.f };
static constexpr SkScalar TypeColumn { 0.36f }; // Of the table's width

/* ----------------------------------------- DeviceRow ----------------------------------------- */

DeviceRow::DeviceRow(SoundPane &pane, AKNode *parent) noexcept :
    AKSolidColor(0, parent),
    m_pane(pane)
{
    const SkRegion empty;
    name.setInputRegion(&empty);
    typeColumn.setInputRegion(&empty);
    type.setInputRegion(&empty);

    layout().setFlexDirection(YGFlexDirectionRow);
    layout().setAlignItems(YGAlignCenter);
    layout().setHeight(TableRowHeight);
    layout().setPadding(YGEdgeHorizontal, AKFormRow::Padding);
    layout().setFlexShrink(0.f);
    name.setFontSize(13.f);
    name.layout().setFlex(1.f);
    name.layout().setFlexShrink(1.f);
    type.setFontSize(13.f);
    typeColumn.layout().setWidthPercent(TypeColumn * 100.f);
    typeColumn.layout().setFlexShrink(0.f);
    updateColors();
}

void DeviceRow::set(const SVAudioDevice &d, bool selected) noexcept
{
    device = d.name;
    name.setText(d.displayName);
    type.setText(d.type);

    if (m_selected != selected)
    {
        m_selected = selected;
        updateColors();
    }
}

void DeviceRow::pointerButtonEvent(const CZPointerButtonEvent &e)
{
    AKSolidColor::pointerButtonEvent(e);

    if (e.button == BTN_LEFT && e.pressed && !m_selected)
        m_pane.deviceClicked(device);
}

void DeviceRow::colorSchemeEvent(const CZColorSchemeEvent &e)
{
    AKSolidColor::colorSchemeEvent(e);
    updateColors();
}

void DeviceRow::updateColors() noexcept
{
    setColor(m_selected ? AKTheme::SelectedContentBackground : CZAdaptiveColor(0));
    name.setTextColor(m_selected ? AKTheme::White : AKTheme::Label);
    type.setTextColor(m_selected ? AKTheme::White : AKTheme::Label);
}

/* ---------------------------------------- VolumeSlider ---------------------------------------- */

VolumeSlider::VolumeSlider(AKNode *parent) noexcept :
    AKContainer(YGFlexDirectionRow, false, parent)
{
    layout().setAlignItems(YGAlignCenter);
    layout().setGap(YGGutterColumn, 6.f);

    const SkRegion empty;
    quiet.setInputRegion(&empty);
    loud.setInputRegion(&empty);

    slider.setStyle(AKSlider::Style::Linear);
    slider.layout().setWidth(240.f);
    slider.layout().setHeight(20.f);
    updateColors();
}

void VolumeSlider::updateColors() noexcept
{
    quiet.setColor(AKTheme::SecondaryLabel.resolve(colorScheme()));
    loud.setColor(AKTheme::SecondaryLabel.resolve(colorScheme()));
}

/* ----------------------------------------- SoundPane ----------------------------------------- */

SoundPane::SoundPane() noexcept : Pane("sound", "Sound")
{
    const SkRegion empty;
    m_alerts = AlertSettings::Load();

    // Alert sound
    {
        std::vector<std::string> names;
        size_t selected { 0 };

        for (size_t i = 0; i < AlertSettings::Sounds().size(); i++)
        {
            names.push_back(AlertSettings::Sounds()[i].name);

            if (AlertSettings::Sounds()[i].id == m_alerts.sound)
                selected = i;
        }

        alertSound.setItems(names, names.empty() ? AKPopUpButton::None : selected);
        alertSound.setEnabled(!names.empty());
        playAlert.setEnabled(!names.empty());
    }

    alertSound.onSelected.subscribe(this, [this](size_t i){
        m_alerts.sound = AlertSettings::Sounds()[i].id;
        saveAlerts();
        playAlertSound();
    });

    playAlert.onClick.subscribe(this, [this](const auto &){ playAlertSound(); });

    alertDevice.onSelected.subscribe(this, [this](size_t i){
        if (i >= m_alertDeviceItems.size())
            return;

        m_alerts.device = m_alertDeviceItems[i];
        saveAlerts();
        playAlertSound();
    });

    // Alert volume: ticks like macOS's, no fill
    alertVolume.slider.setTickCount(8);
    alertVolume.slider.setFillColor(CZAdaptiveColor(0x00000000, 0x00000000));
    alertVolume.slider.setValue(m_alerts.volume);
    alertVolume.slider.onValueChanged.subscribe(this, [this](SkScalar v){ m_alerts.volume = v; });
    alertVolume.slider.onReleased.subscribe(this, [this](SkScalar v){
        m_alerts.volume = v;
        saveAlerts();
        playAlertSound();
    });

    startup.setOn(m_alerts.startupSound, false);
    startup.onToggled.subscribe(this, [this](bool on){ m_alerts.startupSound = on; saveAlerts(); });
    interfaceSounds.setOn(m_alerts.interfaceSounds, false);
    interfaceSounds.onToggled.subscribe(this, [this](bool on){ m_alerts.interfaceSounds = on; saveAlerts(); });
    feedback.setOn(m_alerts.volumeFeedback, false);
    feedback.onToggled.subscribe(this, [this](bool on){ m_alerts.volumeFeedback = on; saveAlerts(); });

    // Output & Input: [Output | Input], then the table (its header, the devices)
    tabs.layout().setPadding(YGEdgeAll, 10.f);
    tab.layout().setWidthPercent(100.f);
    tab.onSelected.subscribe(this, [this](size_t){
        update();
        updateLevelMeter();
    });

    header.layout().setFlexDirection(YGFlexDirectionRow);
    header.layout().setAlignItems(YGAlignCenter);
    header.layout().setHeight(26.f);
    header.layout().setPadding(YGEdgeHorizontal, AKFormRow::Padding);
    header.setInputRegion(&empty);
    headerName.setInputRegion(&empty);
    headerType.setInputRegion(&empty);
    headerName.setFontSize(11.f);
    headerName.layout().setFlex(1.f);
    headerTypeColumn.setInputRegion(&empty);
    headerType.setFontSize(11.f);
    headerTypeColumn.layout().setWidthPercent(TypeColumn * 100.f);
    headerTypeColumn.layout().setFlexShrink(0.f);

    rows.layout().setPadding(YGEdgeBottom, 6.f); // The selected row's fill clear of the round corners
    emptyText.setFontSize(13.f);
    emptyText.layout().setMargin(YGEdgeAll, AKFormRow::Padding);

    // The selected device's volume (and mute or level)
    volume.slider.onValueChanged.subscribe(this, [this](SkScalar v){
        sound->setVolume(inputTab(), v);
    });

    mute.onToggled.subscribe(this, [this](bool on){
        sound->setMuted(false, on);
    });

    sound->onChanged.subscribe(this, [this]{
        update();
        updateLevelMeter();
    });

    sound->onInputLevel.subscribe(this, [this](float v){ level.setValue(v); });

    unavailable.setFontSize(13.f);
    unavailable.setVisible(false);

    update();
    updateColors();
}

void SoundPane::paneShown() noexcept
{
    m_shown = true;
    updateLevelMeter();
}

void SoundPane::paneHidden() noexcept
{
    m_shown = false;
    updateLevelMeter();
}

void SoundPane::deviceClicked(const std::string &device) noexcept
{
    sound->select(inputTab(), device);
}

void SoundPane::update() noexcept
{
    const auto &s { sound->state() };
    const bool input { inputTab() };
    const bool available { !s.loaded || s.available };

    unavailable.setVisible(!available);
    devices.setVisible(available);
    levels.setVisible(available);
    alertDeviceRow.setVisible(available);

    updateAlertDevices();
    updateTable();

    // The selected device's volume
    const SVAudioSelection &sel { input ? s.input : s.output };
    const bool selected { sel.valid && sel.volume >= 0.f };
    volumeRow.title.setText(input ? "Input volume" : "Output volume");
    volumeRow.setVisible(true);
    volume.slider.setValue(selected ? std::min(sel.volume, 1.f) : 0.f);
    muteRow.setVisible(!input);
    mute.setEnabled(selected);

    if (!input && selected && mute.on() != sel.muted)
        mute.setOn(sel.muted);

    levelRow.setVisible(input);
}

void SoundPane::updateAlertDevices() noexcept
{
    const auto &s { sound->state() };
    std::vector<std::string> items { "Selected Sound Output Device" };
    m_alertDeviceItems = { "" };
    size_t selected { 0 };

    for (const auto &d : s.outputs)
    {
        if (d.name == m_alerts.device)
            selected = items.size();

        items.push_back(d.displayName);
        m_alertDeviceItems.push_back(d.name);
    }

    // Saved, but not connected now: kept (and shown) until another one is chosen
    if (!m_alerts.device.empty() && selected == 0 && s.loaded)
    {
        selected = items.size();
        items.push_back(m_alerts.device);
        m_alertDeviceItems.push_back(m_alerts.device);
    }

    alertDevice.setItems(items, selected);
}

void SoundPane::updateTable() noexcept
{
    const auto &s { sound->state() };
    const bool input { inputTab() };
    const auto &list { input ? s.inputs : s.outputs };
    const std::string &selected { input ? s.input.device : s.output.device };

    while (m_rows.size() > list.size())
        m_rows.pop_back();

    while (m_rows.size() < list.size())
        m_rows.emplace_back(std::make_unique<DeviceRow>(*this, &rows));

    for (size_t i = 0; i < list.size(); i++)
        m_rows[i]->set(list[i], list[i].name == selected);

    emptyText.setText(!s.loaded ? "Loading…" : input ? "No input devices found." : "No output devices found.");
    emptyText.setVisible(list.empty());
}

void SoundPane::updateLevelMeter() noexcept
{
    const auto &s { sound->state() };
    const std::string device { m_shown && inputTab() && s.input.valid ? s.input.node : "" };

    if (device == m_levelDevice)
        return;

    m_levelDevice = device;
    level.setValue(0.f);

    if (device.empty())
        sound->stopInputLevel();
    else
        sound->startInputLevel(device);
}

void SoundPane::saveAlerts() noexcept
{
    m_alerts.save();
}

void SoundPane::playAlertSound() noexcept
{
    sound->play(AlertSettings::SoundPath(m_alerts.sound), m_alerts.device, m_alerts.volume);
}

void SoundPane::colorSchemeEvent(const CZColorSchemeEvent &e)
{
    Pane::colorSchemeEvent(e);
    updateColors();
}

void SoundPane::updateColors() noexcept
{
    header.setColor(AKFormSection::CardColor);
    headerName.setTextColor(AKTheme::SecondaryLabel);
    headerType.setTextColor(AKTheme::SecondaryLabel);
    emptyText.setTextColor(AKTheme::SecondaryLabel);
    unavailable.setTextColor(AKTheme::SecondaryLabel);
    alertVolume.updateColors();
    volume.updateColors();
}
