#include <Nodes/SteppedSlider.h>
#include <CZ/AK/AKTheme.h>
#include <algorithm>
#include <cmath>

using namespace CZ;

SteppedSlider::SteppedSlider(const std::string &lowText, const std::string &highText, UInt32 steps, AKNode *parent) noexcept :
    AKContainer(YGFlexDirectionRow, false, parent),
    low(lowText, this),
    high(highText, this),
    m_steps(std::max<UInt32>(2, steps))
{
    layout().setAlignItems(YGAlignCenter);
    layout().setGap(YGGutterColumn, 8.f);

    const SkRegion empty;
    low.setInputRegion(&empty);
    high.setInputRegion(&empty);
    low.setFontSize(11.f);
    high.setFontSize(11.f);
    // Fixed widths (even if empty): the sliders of a pane line up
    low.layout().setWidth(36.f);
    high.layout().setWidth(36.f);
    high.insertAfter(&slider);

    slider.setStyle(AKSlider::Style::Linear);
    slider.setTickCount(m_steps);
    slider.setFillColor(CZAdaptiveColor(0x00000000, 0x00000000));
    slider.layout().setWidth(200.f);
    slider.layout().setHeight(20.f);

    // Snaps to the closest tick once released
    slider.onReleased.subscribe(this, [this](SkScalar v){
        const UInt32 step { UInt32(std::lround(v * SkScalar(m_steps - 1))) };
        slider.setValue(SkScalar(step) / SkScalar(m_steps - 1));

        if (step != m_step)
        {
            m_step = step;

            if (onStepChanged)
                onStepChanged(step);
        }
    });

    updateColors();
}

void SteppedSlider::setStep(UInt32 index) noexcept
{
    m_step = std::min(index, m_steps - 1);
    slider.setValue(SkScalar(m_step) / SkScalar(m_steps - 1));
}

void SteppedSlider::setSteps(UInt32 steps) noexcept
{
    m_steps = std::max<UInt32>(2, steps);
    slider.setTickCount(m_steps);
    setStep(m_step);
}

void SteppedSlider::updateColors() noexcept
{
    low.setTextColor(AKTheme::SecondaryLabel);
    high.setTextColor(AKTheme::SecondaryLabel);
}
