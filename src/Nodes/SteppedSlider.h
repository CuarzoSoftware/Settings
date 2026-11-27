#ifndef STEPPEDSLIDER_H
#define STEPPEDSLIDER_H

#include <CZ/AK/Nodes/AKContainer.h>
#include <CZ/AK/Nodes/AKSlider.h>
#include <CZ/AK/Nodes/AKText.h>
#include <functional>
#include <string>

/// A slider between two captions (e.g. "Off" … "Fast"), snapping to its ticks when released.
struct SteppedSlider : public CZ::AKContainer
{
    SteppedSlider(const std::string &low, const std::string &high, CZ::UInt32 steps, CZ::AKNode *parent) noexcept;

    /// Selects step @p index (0 … steps - 1) without emitting onStepChanged.
    void setStep(CZ::UInt32 index) noexcept;
    CZ::UInt32 step() const noexcept { return m_step; }
    void setSteps(CZ::UInt32 steps) noexcept;
    void updateColors() noexcept;

    CZ::AKText low;
    CZ::AKSlider slider { this };
    CZ::AKText high;
    std::function<void(CZ::UInt32)> onStepChanged; ///< The user chose another step

private:
    CZ::UInt32 m_steps, m_step { 0 };
};

#endif // STEPPEDSLIDER_H
