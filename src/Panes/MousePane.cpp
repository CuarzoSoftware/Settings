#include <Panes/MousePane.h>
#include <CZ/Core/Events/CZColorSchemeEvent.h>
#include <algorithm>
#include <cmath>
#include <iterator>

using namespace CZ;

// The step of @p values closest to @p value
template <class T, size_t N>
static UInt32 ClosestStep(const T (&values)[N], T value) noexcept
{
    UInt32 best { 0 };

    for (UInt32 i = 1; i < N; i++)
        if (std::abs(values[i] - value) < std::abs(values[best] - value))
            best = i;

    return best;
}

MousePane::MousePane() noexcept : Pane("mouse", "Mouse")
{
    speed.onStepChanged = [this](UInt32 step){
        m_settings.trackingSpeed = -1. + 2. * double(step) / double(SpeedSteps - 1);
        m_settings.save();
    };

    naturalRow.setSubtitle("Content tracks finger movement");
    natural.onToggled.subscribe(this, [this](bool on){ m_settings.naturalScrolling = on; m_settings.save(); });

    doubleClick.onStepChanged = [this](UInt32 step){
        m_settings.doubleClickMs = DoubleClickTimes[std::min<size_t>(step, std::size(DoubleClickTimes) - 1)];
        m_settings.save();
        m_settings.exportDoubleClick(); // GTK apps
    };

    scroll.onStepChanged = [this](UInt32 step){
        m_settings.scrollSpeed = ScrollSpeeds[std::min<size_t>(step, std::size(ScrollSpeeds) - 1)];
        m_settings.save();
    };

    load();
    updateColors();
}

void MousePane::paneShown() noexcept
{
    load();
}

void MousePane::load() noexcept
{
    m_settings = MouseSettings::Load();

    const double step { std::round((m_settings.trackingSpeed + 1.) * 0.5 * double(SpeedSteps - 1)) };
    speed.setStep(UInt32(std::clamp(step, 0., double(SpeedSteps - 1))));
    natural.setOn(m_settings.naturalScrolling, false);
    doubleClick.setStep(ClosestStep(DoubleClickTimes, Int32(m_settings.doubleClickMs)));
    scroll.setStep(ClosestStep(ScrollSpeeds, m_settings.scrollSpeed));
}

void MousePane::colorSchemeEvent(const CZColorSchemeEvent &e)
{
    Pane::colorSchemeEvent(e);
    updateColors();
}

void MousePane::updateColors() noexcept
{
    speed.updateColors();
    doubleClick.updateColors();
    scroll.updateColors();
}
