#ifndef SOUNDPANE_H
#define SOUNDPANE_H

#include <Panes/Pane.h>
#include <Backend/AlertSettings.h>
#include <CZ/Services/SAudio.h>
#include <CZ/AK/Nodes/AKFormSection.h>
#include <CZ/AK/Nodes/AKFormRow.h>
#include <CZ/AK/Nodes/AKIconButton.h>
#include <CZ/AK/Nodes/AKFontIcon.h>
#include <CZ/AK/Nodes/AKSwitch.h>
#include <CZ/AK/Nodes/AKSlider.h>
#include <CZ/AK/Nodes/AKSegmentedControl.h>
#include <CZ/AK/Nodes/AKLevelIndicator.h>
#include <CZ/AK/Nodes/AKSolidColor.h>
#include <CZ/AK/Nodes/AKText.h>
#include <CZ/Marco/Nodes/MPopUpButton.h>
#include <memory>
#include <vector>

class SoundPane;

/// A device of the Output & Input table: [name][type]. Clicking it makes it the default one.
class DeviceRow final : public CZ::AKSolidColor
{
public:
    DeviceRow(SoundPane &pane, CZ::AKNode *parent) noexcept;

    void set(const CZ::SAudioDevice &device, bool selected) noexcept;

    std::string device; // Its name
    CZ::AKText name { "", this };
    CZ::AKContainer typeColumn { YGFlexDirectionRow, false, this };
    CZ::AKText type { "", &typeColumn };

protected:
    void pointerButtonEvent(const CZ::CZPointerButtonEvent &e) override;
    void colorSchemeEvent(const CZ::CZColorSchemeEvent &e) override;

private:
    void updateColors() noexcept;
    SoundPane &m_pane;
    bool m_selected { false };
};

/// A volume slider between a quiet and a loud speaker icon (like macOS's).
struct VolumeSlider : public CZ::AKContainer
{
    VolumeSlider(CZ::AKNode *parent) noexcept;
    CZ::AKFontIcon quiet { "volume_mute", 16, this };
    CZ::AKSlider slider { this };
    CZ::AKFontIcon loud { "volume_up", 16, this };
    void updateColors() noexcept;
};

/**
 * @brief Sound: the alert sound and sound effects, and the output and input devices (which one is
 *        used, its volume, and the input level).
 */
class SoundPane final : public Pane
{
public:
    SoundPane() noexcept;

    void paneShown() noexcept override;
    void paneHidden() noexcept override;

    /// The table's row of @p device was clicked.
    void deviceClicked(const std::string &device) noexcept;

    CZ::SAudio sound;

    // Alerts and sound effects (played by Crystals)
    CZ::AKFormSection alerts { "", this };
    CZ::AKFormRow alertSoundRow { "Alert sound", &alerts.card };
    CZ::MPopUpButton alertSound { {}, &alertSoundRow.trailing };
    CZ::AKIconButton playAlert { "play_circle", 20, &alertSoundRow.trailing };
    CZ::AKFormRow alertDeviceRow { "Play sound effects through", &alerts.card };
    CZ::MPopUpButton alertDevice { {}, &alertDeviceRow.trailing };
    CZ::AKFormRow alertVolumeRow { "Alert volume", &alerts.card };
    VolumeSlider alertVolume { &alertVolumeRow.trailing };
    CZ::AKFormRow startupRow { "Play sound on startup", &alerts.card };
    CZ::AKSwitch startup { false, &startupRow.trailing };
    CZ::AKFormRow interfaceRow { "Play user interface sound effects", &alerts.card };
    CZ::AKSwitch interfaceSounds { false, &interfaceRow.trailing };
    CZ::AKFormRow feedbackRow { "Play feedback when volume is changed", &alerts.card };
    CZ::AKSwitch feedback { false, &feedbackRow.trailing };

    // Output & Input: the devices of the selected kind
    CZ::AKFormSection devices { "Output & Input", this };
    CZ::AKContainer tabs { YGFlexDirectionRow, false, &devices.card };
    CZ::AKSegmentedControl tab { { "Output", "Input" }, &tabs };
    CZ::AKSolidColor header { 0, &devices.card };
    CZ::AKText headerName { "Name", &header };
    CZ::AKContainer headerTypeColumn { YGFlexDirectionRow, false, &header };
    CZ::AKText headerType { "Type", &headerTypeColumn };
    CZ::AKContainer rows { YGFlexDirectionColumn, false, &devices.card };
    CZ::AKText emptyText { "", &rows };

    // The selected device's volume (output), or volume and level (input)
    CZ::AKFormSection levels { "", this };
    CZ::AKFormRow volumeRow { "Output volume", &levels.card };
    VolumeSlider volume { &volumeRow.trailing };
    CZ::AKFormRow muteRow { "Mute", &levels.card };
    CZ::AKSwitch mute { false, &muteRow.trailing };
    CZ::AKFormRow levelRow { "Input level", &levels.card };
    CZ::AKLevelIndicator level { 15, &levelRow.trailing };

    CZ::AKText unavailable { "Sound settings aren't available: no audio server (PipeWire or PulseAudio) is running.", this };

protected:
    void colorSchemeEvent(const CZ::CZColorSchemeEvent &e) override;

private:
    bool inputTab() const noexcept { return tab.selectedIndex() == 1; }
    void update() noexcept;
    void updateAlertDevices() noexcept;
    void updateTable() noexcept;
    void updateLevelMeter() noexcept;
    void updateColors() noexcept;
    void saveAlerts() noexcept;
    void playAlertSound() noexcept;

    AlertSettings m_alerts;
    std::vector<std::string> m_alertDeviceItems; // alertDevice's items (outputs' names, "" first)
    std::vector<std::unique_ptr<DeviceRow>> m_rows;
    std::string m_levelDevice; // Being measured
    bool m_shown { false };
};

#endif // SOUNDPANE_H
