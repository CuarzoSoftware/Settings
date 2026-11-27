#ifndef MOUSEPANE_H
#define MOUSEPANE_H

#include <Panes/Pane.h>
#include <Backend/Mouse.h>
#include <Nodes/SteppedSlider.h>
#include <CZ/AK/Nodes/AKFormSection.h>
#include <CZ/AK/Nodes/AKFormRow.h>
#include <CZ/AK/Nodes/AKSwitch.h>

/**
 * @brief Mouse: tracking speed, natural scrolling, double click speed and scrolling speed.
 *
 * Saved to MouseSettings: Crystals applies the tracking and scrolling ones right away, apps the
 * double click speed with the next click.
 */
class MousePane final : public Pane
{
public:
    MousePane() noexcept;

    // Steps of the sliders, from left to right
    static constexpr CZ::UInt32 SpeedSteps { 11 };                                             // Slow (-1) … Fast (1), the middle one is 0
    static constexpr CZ::Int32 DoubleClickTimes[] { 1200, 900, 700, 550, 400, 320, 250, 200 }; // Milliseconds, Slow … Fast
    static constexpr double ScrollSpeeds[] { 0.25, 0.5, 0.75, 1., 1.5, 2., 3., 4. };           // Slow … Fast

    CZ::AKFormSection main { "", this };
    CZ::AKFormRow speedRow { "Tracking speed", &main.card };
    SteppedSlider speed { "Slow", "Fast", SpeedSteps, &speedRow.trailing };
    CZ::AKFormRow naturalRow { "Natural scrolling", &main.card };
    CZ::AKSwitch natural { false, &naturalRow.trailing };
    CZ::AKFormRow doubleClickRow { "Double-Click Speed", &main.card };
    SteppedSlider doubleClick { "Slow", "Fast", 8, &doubleClickRow.trailing };
    CZ::AKFormRow scrollRow { "Scrolling Speed", &main.card };
    SteppedSlider scroll { "Slow", "Fast", 8, &scrollRow.trailing };

    /// Read again: the file may have been edited meanwhile.
    void paneShown() noexcept override;

protected:
    void colorSchemeEvent(const CZ::CZColorSchemeEvent &e) override;

private:
    void load() noexcept;
    void updateColors() noexcept;
    MouseSettings m_settings;
};

#endif // MOUSEPANE_H
