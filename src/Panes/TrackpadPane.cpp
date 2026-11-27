#include <Panes/TrackpadPane.h>
#include <CZ/Core/Events/CZColorSchemeEvent.h>
#include <algorithm>
#include <cmath>

using namespace CZ;

TrackpadPane::TrackpadPane() noexcept : Pane("trackpad", "Trackpad")
{
    speed.onStepChanged = [this](UInt32 step){
        m_settings.trackingSpeed = -1. + 2. * double(step) / double(SpeedSteps - 1);
        m_settings.save();
    };

    secondary.setItems({ "Click or Tap with Two Fingers", "Click in Bottom Right Corner" }, 0);
    secondary.onSelected.subscribe(this, [this](size_t i){
        m_settings.secondaryClick = i == 1 ? TrackpadSettings::SecondaryClick::BottomRight : TrackpadSettings::SecondaryClick::TwoFingers;
        m_settings.save();
    });

    tapRow.setSubtitle("Tap with one finger");
    tap.onToggled.subscribe(this, [this](bool on){ m_settings.tapToClick = on; m_settings.save(); });

    naturalRow.setSubtitle("Content tracks finger movement");
    natural.onToggled.subscribe(this, [this](bool on){ m_settings.naturalScrolling = on; m_settings.save(); });

    workspacesRow.setSubtitle("Swipe left or right with three fingers");
    workspaces.onToggled.subscribe(this, [this](bool on){ m_settings.workspaceSwipe = on; m_settings.save(); });

    exposeRow.setSubtitle("Swipe up with three fingers");
    expose.onToggled.subscribe(this, [this](bool on){ m_settings.expose = on; m_settings.save(); });

    desktopRow.setSubtitle("Swipe down with three fingers");
    desktop.onToggled.subscribe(this, [this](bool on){ m_settings.showDesktop = on; m_settings.save(); });

    load();
    speed.updateColors();
}

void TrackpadPane::paneShown() noexcept
{
    load();
}

void TrackpadPane::load() noexcept
{
    m_settings = TrackpadSettings::Load();

    const double step { std::round((m_settings.trackingSpeed + 1.) * 0.5 * double(SpeedSteps - 1)) };
    speed.setStep(UInt32(std::clamp(step, 0., double(SpeedSteps - 1))));
    secondary.setSelectedIndex(m_settings.secondaryClick == TrackpadSettings::SecondaryClick::BottomRight ? 1 : 0);
    tap.setOn(m_settings.tapToClick, false);
    natural.setOn(m_settings.naturalScrolling, false);
    workspaces.setOn(m_settings.workspaceSwipe, false);
    expose.setOn(m_settings.expose, false);
    desktop.setOn(m_settings.showDesktop, false);
}

void TrackpadPane::colorSchemeEvent(const CZColorSchemeEvent &e)
{
    Pane::colorSchemeEvent(e);
    speed.updateColors();
}
