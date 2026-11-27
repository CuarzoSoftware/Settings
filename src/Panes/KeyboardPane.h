#ifndef KEYBOARDPANE_H
#define KEYBOARDPANE_H

#include <Panes/Pane.h>
#include <Backend/Keyboard.h>
#include <Nodes/SteppedSlider.h>
#include <CZ/AK/Nodes/AKFormSection.h>
#include <CZ/AK/Nodes/AKFormRow.h>
#include <CZ/AK/Nodes/AKSlider.h>
#include <CZ/AK/Nodes/AKSolidColor.h>
#include <CZ/AK/Nodes/AKText.h>
#include <CZ/AK/Nodes/AKButton.h>
#include <CZ/AK/Nodes/AKIconButton.h>
#include <CZ/Marco/Nodes/MPopUpButton.h>
#include <CZ/Marco/Roles/MDialog.h>
#include <functional>
#include <memory>
#include <vector>

/// A selectable row of a list (in the Input Sources dialog).
class ListRow final : public CZ::AKSolidColor
{
public:
    ListRow(const std::string &text, CZ::AKNode *parent) noexcept;
    void setSelected(bool selected) noexcept;

    CZ::AKText text;
    std::function<void()> onClicked;

protected:
    void pointerButtonEvent(const CZ::CZPointerButtonEvent &e) override;
    void colorSchemeEvent(const CZ::CZColorSchemeEvent &e) override;

private:
    void updateColors() noexcept;
    bool m_selected { false };
};

/**
 * @brief Keyboard: key repeat, the keyboard backlight and the input sources (layouts).
 *
 * Saved to KeyboardSettings (applied by Crystals right away); the backlight brightness is set
 * through UPower.
 */
class KeyboardPane final : public Pane
{
public:
    KeyboardPane() noexcept;

    // Steps of the sliders, from left to right
    static constexpr CZ::Int32 RepeatRates[] { 0, 2, 6, 12, 20, 30, 40, 60 };       // Off … Fast
    static constexpr CZ::Int32 RepeatDelays[] { 2000, 1200, 800, 500, 350, 200 };  // Long … Short
    static constexpr CZ::Int32 BacklightTimeouts[] { 0, 5, 10, 30, 60, 300 };      // Seconds, 0: never

    KeyboardBacklight backlight;

    CZ::AKFormSection main { "", this };
    CZ::AKFormRow repeatRow { "Key repeat rate", &main.card };
    SteppedSlider repeat { "Off", "Fast", 8, &repeatRow.trailing };
    CZ::AKFormRow delayRow { "Delay until repeat", &main.card };
    SteppedSlider delay { "Long", "Short", 6, &delayRow.trailing };
    CZ::AKFormRow brightnessRow { "Keyboard brightness", &main.card };
    SteppedSlider brightness { "", "", 2, &brightnessRow.trailing };
    CZ::AKFormRow timeoutRow { "Turn keyboard backlight off after inactivity", &main.card };
    CZ::MPopUpButton timeout { {}, &timeoutRow.trailing };

    CZ::AKFormSection textInput { "Text Input", this };
    CZ::AKFormRow sourcesRow { "Input Sources", &textInput.card };
    CZ::AKText sourcesText { "", &sourcesRow.trailing };
    CZ::AKButton editSources { "Edit…", &sourcesRow.trailing };

protected:
    void colorSchemeEvent(const CZ::CZColorSchemeEvent &e) override;

private:
    void updateBacklight() noexcept;
    void updateSources() noexcept;
    void updateColors() noexcept;
    void save() noexcept;

    // The Input Sources dialog
    void openSourcesDialog() noexcept;
    void fillSourcesList() noexcept;
    void fillCatalog(const std::string &filter) noexcept;

    KeyboardSettings m_settings;
    std::unique_ptr<CZ::MDialog> m_dialog;

    struct SourcesDialog
    {
        CZ::AKNode *list {};           // The sources
        CZ::AKContainer *picker {};    // Adding one: the search field and the catalog
        CZ::AKContainer *catalog {};   // Inside a scroll
        CZ::AKIconButton *remove {};
        std::vector<std::unique_ptr<ListRow>> rows, catalogRows;
        size_t selected { 0 };
    } m_sd;
};

#endif // KEYBOARDPANE_H
