#ifndef TRACKPADPANE_H
#define TRACKPADPANE_H

#include <Panes/Pane.h>
#include <Backend/Trackpad.h>
#include <Nodes/SteppedSlider.h>
#include <CZ/AK/Nodes/AKFormSection.h>
#include <CZ/AK/Nodes/AKFormRow.h>
#include <CZ/AK/Nodes/AKSwitch.h>
#include <CZ/Marco/Nodes/MPopUpButton.h>

/**
 * @brief Trackpad: pointing, clicking, scrolling and the three finger swipes.
 *
 * Saved to TrackpadSettings (applied by Crystals right away).
 */
class TrackpadPane final : public Pane
{
public:
    TrackpadPane() noexcept;

    static constexpr CZ::UInt32 SpeedSteps { 11 }; // Slow (-1) … Fast (1), the middle one is 0

    CZ::AKFormSection main { "", this };
    CZ::AKFormRow speedRow { "Tracking speed", &main.card };
    SteppedSlider speed { "Slow", "Fast", SpeedSteps, &speedRow.trailing };
    CZ::AKFormRow secondaryRow { "Secondary click", &main.card };
    CZ::MPopUpButton secondary { {}, &secondaryRow.trailing };
    CZ::AKFormRow tapRow { "Tap to click", &main.card };
    CZ::AKSwitch tap { false, &tapRow.trailing };
    CZ::AKFormRow naturalRow { "Natural scrolling", &main.card };
    CZ::AKSwitch natural { false, &naturalRow.trailing };
    CZ::AKFormRow workspacesRow { "Swipe between desktops", &main.card };
    CZ::AKSwitch workspaces { false, &workspacesRow.trailing };
    CZ::AKFormRow exposeRow { "Exposé", &main.card };
    CZ::AKSwitch expose { false, &exposeRow.trailing };
    CZ::AKFormRow desktopRow { "Show Desktop", &main.card };
    CZ::AKSwitch desktop { false, &desktopRow.trailing };

    /// Read again: the file may have been edited meanwhile.
    void paneShown() noexcept override;

protected:
    void colorSchemeEvent(const CZ::CZColorSchemeEvent &e) override;

private:
    void load() noexcept;
    TrackpadSettings m_settings;
};

#endif // TRACKPADPANE_H
