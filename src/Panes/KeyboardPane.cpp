#include <Panes/KeyboardPane.h>
#include <CZ/AK/AKTheme.h>
#include <CZ/AK/AKScene.h>
#include <CZ/AK/Nodes/AKRoundSolidColor.h>
#include <CZ/AK/Nodes/AKIconButton.h>
#include <CZ/AK/Nodes/AKSearchField.h>
#include <CZ/AK/Nodes/AKScroll.h>
#include <CZ/Marco/Roles/MToplevel.h>
#include <CZ/Core/Events/CZColorSchemeEvent.h>
#include <CZ/Core/Events/CZPointerButtonEvent.h>
#include <linux/input-event-codes.h>
#include <algorithm>
#include <cctype>
#include <cmath>

using namespace CZ;

static constexpr size_t MaxCatalogRows { 100 };

// The step of @p values closest to @p value
template <size_t N>
static UInt32 ClosestStep(const Int32 (&values)[N], Int32 value) noexcept
{
    UInt32 best { 0 };

    for (UInt32 i = 1; i < N; i++)
        if (std::abs(values[i] - value) < std::abs(values[best] - value))
            best = i;

    return best;
}

static std::string Lower(std::string s) noexcept
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
    return s;
}

/* ------------------------------------------ ListRow ------------------------------------------ */

ListRow::ListRow(const std::string &label, AKNode *parent) noexcept :
    AKSolidColor(0, parent),
    text(label, this)
{
    const SkRegion empty;
    text.setInputRegion(&empty);
    text.setFontSize(13.f);
    layout().setFlexDirection(YGFlexDirectionRow);
    layout().setAlignItems(YGAlignCenter);
    layout().setHeight(26.f);
    layout().setPadding(YGEdgeHorizontal, 10.f);
    layout().setFlexShrink(0.f);
    updateColors();
}

void ListRow::setSelected(bool selected) noexcept
{
    if (m_selected == selected)
        return;

    m_selected = selected;
    updateColors();
}

void ListRow::pointerButtonEvent(const CZPointerButtonEvent &e)
{
    AKSolidColor::pointerButtonEvent(e);

    if (e.button == BTN_LEFT && !e.pressed && onClicked)
        onClicked();
}

void ListRow::colorSchemeEvent(const CZColorSchemeEvent &e)
{
    AKSolidColor::colorSchemeEvent(e);
    updateColors();
}

void ListRow::updateColors() noexcept
{
    setColor(m_selected ? AKTheme::SelectedContentBackground : CZAdaptiveColor(0));
    text.setTextColor(m_selected ? AKTheme::White : AKTheme::Label);
}

/* ---------------------------------------- KeyboardPane ---------------------------------------- */

KeyboardPane::KeyboardPane() noexcept : Pane("keyboard", "Keyboard")
{
    m_settings = KeyboardSettings::Load();

    // Key repeat
    repeat.setStep(ClosestStep(RepeatRates, m_settings.repeatRate));
    repeat.onStepChanged = [this](UInt32 step){
        m_settings.repeatRate = RepeatRates[step];
        save();
    };

    delay.setStep(ClosestStep(RepeatDelays, m_settings.repeatDelay));
    delay.onStepChanged = [this](UInt32 step){
        m_settings.repeatDelay = RepeatDelays[step];
        save();
    };

    // The backlight (rows hidden without one)
    brightness.onStepChanged = [this](UInt32 step){ backlight.setBrightness(Int32(step)); };
    backlight.onChanged.subscribe(this, [this]{ updateBacklight(); });

    timeout.setItems({ "Never", "After 5 Seconds", "After 10 Seconds", "After 30 Seconds", "After 1 Minute", "After 5 Minutes" },
                     ClosestStep(BacklightTimeouts, m_settings.backlightTimeout));
    timeout.onSelected.subscribe(this, [this](size_t i){
        m_settings.backlightTimeout = BacklightTimeouts[i];
        save();
    });

    // Input sources
    const SkRegion empty;
    sourcesText.setInputRegion(&empty);
    sourcesText.setFontSize(13.f);
    editSources.setBackgroundColor(CZAdaptiveColor(0xFFFFFFFF, 0x33FFFFFF)); // Plain (not the accent)
    editSources.setContentColor(AKTheme::ControlText);
    editSources.onClick.subscribe(this, [this](const auto &){ openSourcesDialog(); });

    updateBacklight();
    updateSources();
    updateColors();
}

void KeyboardPane::save() noexcept
{
    m_settings.save();
}

void KeyboardPane::updateBacklight() noexcept
{
    brightnessRow.setVisible(backlight.available());
    timeoutRow.setVisible(backlight.available());

    if (!backlight.available())
        return;

    brightness.setSteps(UInt32(backlight.max()) + 1);

    if (!brightness.slider.dragging())
        brightness.setStep(UInt32(backlight.brightness()));
}

void KeyboardPane::updateSources() noexcept
{
    // "Spanish (Latin American)", or the first one and how many more
    std::string text { XkbCatalog::Name(m_settings.sources.front()) };

    if (m_settings.sources.size() == 2)
        text += ", " + XkbCatalog::Name(m_settings.sources[1]);
    else if (m_settings.sources.size() > 2)
        text += " and " + std::to_string(m_settings.sources.size() - 1) + " more";

    sourcesText.setText(text);
    sourcesRow.setSubtitle(m_settings.sources.size() > 1 ? "Press Super+Space to switch between them." : "");
}

/* ------------------------------------ Input Sources dialog ------------------------------------ */

void KeyboardPane::openSourcesDialog() noexcept
{
    auto *window { scene() ? dynamic_cast<MToplevel*>(scene()->window()) : nullptr };

    if (!window)
        return;

    // The rows belong to the previous dialog's nodes: gone first
    m_sd.rows.clear();
    m_sd.catalogRows.clear();

    if (m_dialog)
        m_dialog->close();

    m_dialog = std::make_unique<MDialog>("Input Sources", "Keyboard layouts you can type with. With several, Super+Space switches between them.");
    MDialog &d { *m_dialog };
    d.layout().setWidth(420.f);
    d.content.layout().setGap(YGGutterRow, 8.f);

    // The sources, in a card
    auto &card { d.own(std::make_unique<AKRoundSolidColor>(0, CZBorderRadius::Make(8), &d.content)) };
    card.setBackgroundColor(AKFormSection::CardColor.resolve(colorScheme()));
    card.setStrokeColor(AKFormSection::CardBorder.resolve(colorScheme()));
    card.setStrokeWidth(1);
    card.enableChildrenClipping(true);
    card.layout().setFlexDirection(YGFlexDirectionColumn);
    card.layout().setPadding(YGEdgeVertical, 4.f);
    card.layout().setMinHeight(90.f);
    m_sd.list = &card;

    // [+][-]
    auto &bar { d.own(std::make_unique<AKContainer>(YGFlexDirectionRow, false, &d.content)) };
    bar.layout().setGap(YGGutterColumn, 4.f);
    auto &add { d.own(std::make_unique<AKIconButton>("add", 18, &bar)) };
    auto &remove { d.own(std::make_unique<AKIconButton>("remove", 18, &bar)) };
    add.setOutlined(true);
    remove.setOutlined(true);
    m_sd.remove = &remove; // Disabled while there's a single source (it can't be removed)
    m_sd.selected = 0;

    // Adding one: search the layouts (hidden until +)
    auto &picker { d.own(std::make_unique<AKContainer>(YGFlexDirectionColumn, false, &d.content)) };
    picker.layout().setGap(YGGutterRow, 6.f);
    picker.setVisible(false);
    m_sd.picker = &picker;
    auto &search { d.own(std::make_unique<AKSearchField>("Search layouts", &picker)) };
    auto &catalogCard { d.own(std::make_unique<AKRoundSolidColor>(0, CZBorderRadius::Make(8), &picker)) };
    catalogCard.setBackgroundColor(AKFormSection::CardColor.resolve(colorScheme()));
    catalogCard.setStrokeColor(AKFormSection::CardBorder.resolve(colorScheme()));
    catalogCard.setStrokeWidth(1);
    catalogCard.enableChildrenClipping(true);
    catalogCard.layout().setHeight(220.f);
    auto &scroll { d.own(std::make_unique<AKScroll>(&catalogCard)) };
    scroll.layout().setFlex(1.f);
    auto &catalog { d.own(std::make_unique<AKContainer>(YGFlexDirectionColumn, false, &scroll)) };
    catalog.layout().setWidthPercent(100.f);
    catalog.layout().setPadding(YGEdgeVertical, 4.f);
    m_sd.catalog = &catalog;

    add.onClick.subscribe(&d, [this, &search](const auto &){
        m_sd.picker->setVisible(!m_sd.picker->visible());
        fillCatalog(search.text());
    });

    remove.onClick.subscribe(&d, [this](const auto &){
        if (m_settings.sources.size() <= 1 || m_sd.selected >= m_settings.sources.size())
            return;

        m_settings.sources.erase(m_settings.sources.begin() + Int64(m_sd.selected));
        m_sd.selected = std::min(m_sd.selected, m_settings.sources.size() - 1);
        save();
        updateSources();
        fillSourcesList();
    });

    search.onTextChanged.subscribe(&d, [this](const std::string &text){ fillCatalog(text); });

    d.addButton("Done", MDialog::Role::Default);
    fillSourcesList();
    d.open(*window);
}

void KeyboardPane::fillSourcesList() noexcept
{
    if (!m_sd.list)
        return;

    m_sd.rows.clear();

    for (size_t i = 0; i < m_settings.sources.size(); i++)
    {
        auto &row { m_sd.rows.emplace_back(std::make_unique<ListRow>(XkbCatalog::Name(m_settings.sources[i]), m_sd.list)) };
        row->setSelected(i == m_sd.selected);
        row->onClicked = [this, i]{
            m_sd.selected = i;

            for (size_t j = 0; j < m_sd.rows.size(); j++)
                m_sd.rows[j]->setSelected(j == i);
        };
    }

    if (m_sd.remove)
        m_sd.remove->setEnabled(m_settings.sources.size() > 1);
}

void KeyboardPane::fillCatalog(const std::string &filter) noexcept
{
    if (!m_sd.catalog || !m_sd.picker->visible())
        return;

    m_sd.catalogRows.clear();
    const std::string f { Lower(filter) };

    for (const auto &entry : XkbCatalog::Entries())
    {
        if (m_sd.catalogRows.size() >= MaxCatalogRows)
            break;

        if (std::find(m_settings.sources.begin(), m_settings.sources.end(), entry.source) != m_settings.sources.end())
            continue;

        if (!f.empty() && Lower(entry.name).find(f) == std::string::npos && Lower(entry.source.layout) != f)
            continue;

        auto &row { m_sd.catalogRows.emplace_back(std::make_unique<ListRow>(entry.name, m_sd.catalog)) };
        const InputSource source { entry.source };
        row->onClicked = [this, source]{
            m_settings.sources.push_back(source);
            m_sd.selected = m_settings.sources.size() - 1;
            save();
            updateSources();
            fillSourcesList();
            m_sd.picker->setVisible(false);
        };
    }
}

void KeyboardPane::colorSchemeEvent(const CZColorSchemeEvent &e)
{
    Pane::colorSchemeEvent(e);
    updateColors();
}

void KeyboardPane::updateColors() noexcept
{
    sourcesText.setTextColor(AKTheme::SecondaryLabel);
    repeat.updateColors();
    delay.updateColors();
    brightness.updateColors();
}
