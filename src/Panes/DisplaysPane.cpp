#include <Panes/DisplaysPane.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/Roles/MToplevel.h>
#include <CZ/AK/AKScene.h>
#include <CZ/AK/AKApp.h>
#include <CZ/AK/AKTheme.h>
#include <CZ/Core/CZWeak.h>
#include <CZ/Core/Events/CZLayoutEvent.h>
#include <CZ/Core/Events/CZColorSchemeEvent.h>
#include <CZ/Core/Events/CZPointerButtonEvent.h>
#include <CZ/Core/Events/CZPointerMoveEvent.h>
#include <linux/input-event-codes.h>
#include <algorithm>
#include <cmath>
#include <format>

using namespace CZ;

static const SkFontStyle Bold { SkFontStyle::kBold_Weight, SkFontStyle::kNormal_Width, SkFontStyle::kUpright_Slant };

static const std::vector<std::string> TransformNames {
    "Standard", "90°", "180°", "270°", "Flipped", "Flipped 90°", "Flipped 180°", "Flipped 270°"
};

static std::string RefreshText(Int32 mHz) noexcept
{
    if (mHz <= 0)
        return "";

    if (mHz % 1000 == 0)
        return std::format("{} Hz", mHz / 1000);

    return std::format("{:.2f} Hz", mHz / 1000.0);
}

static std::string ModeText(const MOutputMode &mode) noexcept
{
    std::string text { std::format("{} × {}", mode.size().width(), mode.size().height()) };

    if (mode.refreshRate() > 0)
        text += " @ " + RefreshText(mode.refreshRate());

    if (mode.preferred())
        text += " (Default)";

    return text;
}

static std::string ScaleText(Float64 scale) noexcept
{
    std::string text { std::format("{:.2f}", scale) };

    // 1.50 -> 1.5, 2.00 -> 2
    while (text.back() == '0')
        text.pop_back();

    if (text.back() == '.')
        text.pop_back();

    return text + "×";
}

/* -------------------------------------------- Box -------------------------------------------- */

DisplayBox::DisplayBox(DisplaysPane &pane, MOutputHead &head, AKNode *parent) noexcept :
    AKRoundSolidColor(0, CZBorderRadius::Make(6), parent),
    head(head),
    m_pane(pane)
{
    layout().setPositionType(YGPositionTypeAbsolute);
    layout().setFlexDirection(YGFlexDirectionColumn);
    layout().setJustifyContent(YGJustifyCenter);
    layout().setAlignItems(YGAlignCenter);
    layout().setGap(YGGutterRow, 2.f);
    enableChildrenClipping(true);

    const SkRegion empty;
    name.setInputRegion(&empty);
    name.setFontSize(12.f);
    name.setFontStyle(Bold);
    name.setText(head.props().name);
    size.setInputRegion(&empty);
    size.setFontSize(10.f);
    updateColors();
}

void DisplayBox::setSelected(bool selected) noexcept
{
    m_selected = selected;
    updateColors();
}

void DisplayBox::pointerButtonEvent(const CZPointerButtonEvent &e)
{
    AKRoundSolidColor::pointerButtonEvent(e);

    if (e.button != BTN_LEFT)
        return;

    if (e.pressed)
    {
        m_pane.select(&head);

        // Only the ones in use are arranged
        if (!m_pane.pendingEnabled(&head))
            return;

        m_dragging = true;
        const SkPoint pointer { AKApp::Get()->pointer().pos() };
        m_grab = SkPoint::Make(pointer.x() - worldRect().x(), pointer.y() - worldRect().y());
        setParent(parent()); // Above the others while dragged

        if (scene())
            scene()->setPointerGrab(this);
    }
    else if (m_dragging)
    {
        m_dragging = false;

        if (scene() && scene()->pointerGrab() == this)
            scene()->setPointerGrab(nullptr);

        m_pane.boxDropped(*this);
    }
}

void DisplayBox::pointerMoveEvent(const CZPointerMoveEvent &e)
{
    AKRoundSolidColor::pointerMoveEvent(e);

    if (!m_dragging || !parent())
        return;

    const SkPoint pointer { AKApp::Get()->pointer().pos() };
    const SkIRect area { parent()->worldRect() };
    m_pane.boxDragged(*this, SkPoint::Make(pointer.x() - area.x() - m_grab.x(), pointer.y() - area.y() - m_grab.y()));
}

void DisplayBox::colorSchemeEvent(const CZColorSchemeEvent &e)
{
    AKRoundSolidColor::colorSchemeEvent(e);
    updateColors();
}

void DisplayBox::updateColors() noexcept
{
    const CZColorScheme scheme { colorScheme() };
    // Opaque
    const SkColor base { AKTheme::ControlBackground.resolve(scheme) };
    const SkColor accent { AKTheme::SystemBlue.resolve(scheme) };
    const auto mix = [](U8CPU a, U8CPU b){ return U8CPU((a * 3 + b) / 4); }; // 25% accent
    setBackgroundColor(m_selected ? SkColorSetRGB(mix(SkColorGetR(base), SkColorGetR(accent)),
                                                  mix(SkColorGetG(base), SkColorGetG(accent)),
                                                  mix(SkColorGetB(base), SkColorGetB(accent))) : base);
    setStrokeColor(m_selected ? AKTheme::ControlAccent.resolve(scheme) : AKTheme::TertiaryLabel.resolve(scheme));
    setStrokeWidth(m_selected ? 2 : 1);
    name.setTextColor(AKTheme::Label);
    size.setTextColor(AKTheme::SecondaryLabel);
}

/* ----------------------------------------- Arrangement --------------------------------------- */

DisplayArrangement::DisplayArrangement(DisplaysPane &pane, AKNode *parent) noexcept :
    AKRoundSolidColor(0, CZBorderRadius::Make(10), parent),
    m_pane(pane)
{
    layout().setHeight(Height);
    layout().setWidthPercent(100.f);
    enableChildrenClipping(true);
    setStrokeWidth(1);
    updateColors();
}

void DisplayArrangement::layoutEvent(const CZLayoutEvent &e)
{
    AKRoundSolidColor::layoutEvent(e);

    if (e.changes.has(CZLayoutChangeSize))
        m_pane.layoutBoxes();
}

void DisplayArrangement::colorSchemeEvent(const CZColorSchemeEvent &e)
{
    AKRoundSolidColor::colorSchemeEvent(e);
    updateColors();
}

void DisplayArrangement::updateColors() noexcept
{
    const CZColorScheme scheme { colorScheme() };
    setBackgroundColor(AKFormSection::CardColor.resolve(scheme));
    setStrokeColor(AKFormSection::CardBorder.resolve(scheme));
}

/* -------------------------------------------- Pane ------------------------------------------- */

DisplaysPane::DisplaysPane() noexcept :
    Pane("displays", "Displays"),
    m_manager(MApp::Get()->ext.outputManager)
{
    unavailable.setFontSize(13.f);
    unavailable.setTextColor(AKTheme::SecondaryLabel);

    enabled.onToggled.subscribe(this, [this](bool on){
        if (m_selected)
            setEnabled(m_selected, on);
    });

    buttons.layout().setJustifyContent(YGJustifyFlexEnd);
    buttons.layout().setMargin(YGEdgeTop, -10.f);

    for (Float64 s = 0.5; s <= 4.0; s += 0.25)
        m_scaleItems.push_back(s);

    std::vector<std::string> scaleNames;

    for (Float64 s : m_scaleItems)
        scaleNames.push_back(ScaleText(s));

    scale.setItems(scaleNames);
    rotation.setItems(TransformNames);

    resolution.onSelected.subscribe(this, [this](size_t i){
        if (!m_selected || i >= m_modeItems.size())
            return;

        m_pending[m_selected].mode = m_modeItems[i];
        normalize();
        layoutBoxes();
        updateControls();
    });

    scale.onSelected.subscribe(this, [this](size_t i){
        if (!m_selected || i >= m_scaleItems.size())
            return;

        m_pending[m_selected].scale = m_scaleItems[i];
        normalize();
        layoutBoxes();
        updateControls();
    });

    rotation.onSelected.subscribe(this, [this](size_t i){
        if (!m_selected || i >= TransformNames.size())
            return;

        m_pending[m_selected].transform = static_cast<CZTransform>(i);
        normalize();
        layoutBoxes();
        updateControls();
    });

    apply.onClick.subscribe(this, [this](const CZPointerButtonEvent &){ applyPending(); });

    m_manager.onChanged.subscribe(this, [this]{
        reload();

        // Cancelled before: the state it was sent against changed, now it's known
        if (m_revertPending)
        {
            m_revertPending = false;
            revert();
        }
    });

    m_manager.onRemoved.subscribe(this, [this](MOutputHead *head){
        // The heads are already without it
        m_pending.erase(head);

        if (m_selected == head)
            m_selected = nullptr;

        reload();
    });

    m_countdown.setCallback([this](CZTimer *){
        if (--m_secondsLeft <= 0)
        {
            if (m_dialog)
                m_dialog->close();

            revert();
            return;
        }

        if (m_dialog)
            m_dialog->setMessage(std::format("The previous settings will be restored in {} seconds.", m_secondsLeft));

        m_countdown.start(1000);
    });

    updateColors();
    reload();
}

void DisplaysPane::reload() noexcept
{
    const bool available { m_manager.available() };
    unavailable.setVisible(!available);
    arrangement.setVisible(available);
    settings.setVisible(available);
    buttons.setVisible(available);

    const std::string selectedName { m_selected ? m_selected->props().name : "" };
    m_selected = nullptr;
    m_pending.clear();
    m_order.clear();
    m_disabled.clear();

    for (auto *head : m_manager.heads())
    {
        m_pending[head] = head->config();

        if (head->props().enabled)
            m_order.push_back(head);
        else
            m_disabled.push_back(head);

        if (head->props().name == selectedName)
            m_selected = head;
    }

    // Where they are now, left to right
    std::stable_sort(m_order.begin(), m_order.end(), [](MOutputHead *a, MOutputHead *b){
        const auto pa { a->props().pos }, pb { b->props().pos };
        return pa.x() != pb.x() ? pa.x() < pb.x() : pa.y() < pb.y();
    });

    if (!m_selected)
        m_selected = !m_order.empty() ? m_order.front() : !m_disabled.empty() ? m_disabled.front() : nullptr;

    normalize();
    rebuildBoxes();
    updateControls();
}

void DisplaysPane::normalize() noexcept
{
    // A row from (0, 0), each one right where the previous one ends
    Int32 x { 0 };

    for (auto *head : m_order)
    {
        m_pending[head].pos = SkIPoint::Make(x, 0);
        x += logicalSize(head).width();
    }
}

SkISize DisplaysPane::logicalSize(MOutputHead *head) const noexcept
{
    const auto it { m_pending.find(head) };

    if (it == m_pending.end())
        return { 0, 0 };

    const auto &config { it->second };
    const MOutputMode *mode { config.mode ? config.mode : head->props().currentMode ? head->props().currentMode :
                              head->preferredMode() ? head->preferredMode() :
                              head->props().modes.empty() ? nullptr : head->props().modes.front() };
    SkISize size { mode ? mode->size() : SkISize::Make(1920, 1080) };

    // Like Louvre (LOutput::size()): fractional scales render at the next integer one, downscaled
    const Float64 fractional { std::max(config.scale, 0.25) };
    const Float32 integer { Float32(std::ceil(fractional)) };
    Float32 w { Float32(size.width()) }, h { Float32(size.height()) };

    if (fractional != integer)
    {
        w = std::round(w * integer / Float32(fractional));
        h = std::round(h * integer / Float32(fractional));
    }

    if (CZ::Is90Transform(config.transform))
        std::swap(w, h);

    return SkISize::Make(Int32(std::round(w / integer)), Int32(std::round(h / integer)));
}

void DisplaysPane::rebuildBoxes() noexcept
{
    m_boxes.clear();

    for (auto *head : m_order)
        m_boxes.emplace_back(std::make_unique<DisplayBox>(*this, *head, &arrangement));

    for (auto *head : m_disabled)
        m_boxes.emplace_back(std::make_unique<DisplayBox>(*this, *head, &arrangement));

    layoutBoxes();
}

void DisplaysPane::layoutBoxes() noexcept
{
    // The calculated size: worldRect() is updated after layoutEvent() (stale when first shown)
    const SkISize area { SkISize::Make(Int32(arrangement.layout().calculatedWidth()), Int32(arrangement.layout().calculatedHeight())) };

    if (area.isEmpty() || m_boxes.empty())
        return;

    // The enabled ones in a row, then (apart, dimmed) the disabled ones
    static constexpr SkScalar Gap { 16.f };
    std::vector<MOutputHead*> heads { m_order };
    heads.insert(heads.end(), m_disabled.begin(), m_disabled.end());
    Int32 total { 0 }, maxHeight { 0 };

    for (auto *head : heads)
    {
        const SkISize size { logicalSize(head) };
        total += size.width();
        maxHeight = std::max(maxHeight, size.height());
    }

    if (total <= 0 || maxHeight <= 0)
        return;

    const SkScalar gaps { m_disabled.empty() ? 0.f : Gap * SkScalar(m_disabled.size() - (m_order.empty() ? 1 : 0)) };
    const SkScalar m { DisplayArrangement::Margin };
    const SkScalar f { std::min({ (area.width() - 2.f * m - gaps) / total, (area.height() - 2.f * m) / maxHeight,
                                  area.height() * 0.6f / maxHeight }) };
    const SkScalar y0 { (area.height() - maxHeight * f) / 2.f };
    SkScalar x { (area.width() - total * f - gaps) / 2.f };

    for (size_t i = 0; i < heads.size(); i++)
    {
        MOutputHead *head { heads[i] };
        const bool on { i < m_order.size() };

        if (!on && (i > 0))
            x += Gap;

        const SkISize size { logicalSize(head) };
        const SkScalar w { size.width() * f }, h { size.height() * f };

        for (auto &box : m_boxes)
        {
            if (&box->head != head)
                continue;

            box->size.setText(on ? std::format("{} × {}", size.width(), size.height()) : "Off");
            box->size.setVisible(h > 44.f);
            box->setSelected(head == m_selected);
            box->setOpacity(on ? 1.f : 0.45f);
            box->layout().setWidth(std::max(w - 2.f, 1.f)); // 1 px apart
            box->layout().setHeight(h);
            box->width = w;

            if (!box->dragging())
            {
                box->x = x;
                box->layout().setPosition(YGEdgeLeft, x + 1.f);
                box->layout().setPosition(YGEdgeTop, y0);
            }
        }

        x += w;
    }
}

void DisplaysPane::boxDragged(DisplayBox &box, SkPoint pos) noexcept
{
    const SkISize area { SkISize::Make(Int32(arrangement.layout().calculatedWidth()), Int32(arrangement.layout().calculatedHeight())) };
    const SkScalar h { box.layout().height().value };
    box.x = std::clamp(pos.x(), 0.f, std::max(0.f, area.width() - box.width));
    box.layout().setPosition(YGEdgeLeft, box.x + 1.f);
    box.layout().setPosition(YGEdgeTop, std::clamp(pos.y(), 0.f, std::max(0.f, area.height() - h)));
}

void DisplaysPane::boxDropped(DisplayBox &) noexcept
{
    // Only the order matters: by where their centers are now
    std::unordered_map<MOutputHead*, SkScalar> centers;

    for (auto &box : m_boxes)
        centers[&box->head] = box->x + box->width / 2.f;

    std::stable_sort(m_order.begin(), m_order.end(), [&centers](MOutputHead *a, MOutputHead *b){
        return centers[a] < centers[b];
    });

    normalize();
    layoutBoxes();
    updateControls();
}

bool DisplaysPane::pendingEnabled(MOutputHead *head) const noexcept
{
    const auto it { m_pending.find(head) };
    return it != m_pending.end() && it->second.enabled;
}

void DisplaysPane::setEnabled(MOutputHead *head, bool on) noexcept
{
    auto &config { m_pending[head] };

    if (config.enabled == on)
        return;

    // At least one stays on
    if (!on && m_order.size() <= 1)
    {
        updateControls();
        return;
    }

    config.enabled = on;
    std::erase(m_order, head);
    std::erase(m_disabled, head);

    if (on)
    {
        // Its preferred mode if it had none (e.g. off since it was plugged)
        if (!config.mode)
            config.mode = head->preferredMode() ? head->preferredMode() :
                          head->props().modes.empty() ? nullptr : head->props().modes.front();

        m_order.push_back(head); // At the right end
    }
    else
    {
        // Listed in the manager's order
        m_disabled.clear();

        for (auto *h : m_manager.heads())
            if (!pendingEnabled(h))
                m_disabled.push_back(h);
    }

    normalize();
    layoutBoxes();
    updateControls();
}

void DisplaysPane::select(MOutputHead *head) noexcept
{
    m_selected = head;

    for (auto &box : m_boxes)
        box->setSelected(&box->head == head);

    updateControls();
}

void DisplaysPane::updateControls() noexcept
{
    apply.setEnabled(dirty());

    const bool has { m_selected && m_pending.contains(m_selected) };
    settings.setVisible(has && m_manager.available());

    if (!has)
        return;

    const auto &config { m_pending[m_selected] };
    enabled.setOn(config.enabled); // Animated back if it was turned off but is the last one
    enabled.setEnabled(!config.enabled || m_order.size() > 1); // The last one stays on
    resolutionRow.setVisible(config.enabled);
    scaleRow.setVisible(config.enabled);
    rotationRow.setVisible(config.enabled);
    settings.updateSeparators();
    settings.setTitle(m_selected->props().make.empty() ? m_selected->props().name :
                      m_selected->props().make + " " + m_selected->props().model);

    // Resolution
    m_modeItems = m_selected->props().modes;
    std::vector<std::string> modeNames;
    size_t modeIndex { AKPopUpButton::None };

    for (size_t i = 0; i < m_modeItems.size(); i++)
    {
        modeNames.push_back(ModeText(*m_modeItems[i]));

        if (m_modeItems[i] == config.mode)
            modeIndex = i;
    }

    resolution.setItems(modeNames, modeIndex);

    // Scale: the current one is listed even if it isn't one of the steps
    if (std::find(m_scaleItems.begin(), m_scaleItems.end(), config.scale) == m_scaleItems.end())
    {
        m_scaleItems.erase(std::remove_if(m_scaleItems.begin(), m_scaleItems.end(), [](Float64 s){
            return std::fmod(s, 0.25) != 0.0; // A previous odd one
        }), m_scaleItems.end());
        m_scaleItems.insert(std::upper_bound(m_scaleItems.begin(), m_scaleItems.end(), config.scale), config.scale);

        std::vector<std::string> scaleNames;

        for (Float64 s : m_scaleItems)
            scaleNames.push_back(ScaleText(s));

        scale.setItems(scaleNames);
    }

    scale.setSelectedIndex(size_t(std::find(m_scaleItems.begin(), m_scaleItems.end(), config.scale) - m_scaleItems.begin()));
    rotation.setSelectedIndex(size_t(config.transform));
}

bool DisplaysPane::dirty() const noexcept
{
    for (const auto &[head, config] : m_pending)
    {
        const auto now { head->config() };

        if (!config.enabled && !now.enabled)
            continue;

        if (config.enabled != now.enabled || config.mode != now.mode || config.pos != now.pos ||
            config.transform != now.transform || config.scale != now.scale)
            return true;
    }

    return false;
}

void DisplaysPane::applyPending() noexcept
{
    if (!dirty())
        return;

    // To go back to (by name: heads may come and go meanwhile)
    m_previous.clear();

    for (auto *head : m_manager.heads())
    {
        const auto &p { head->props() };
        m_previous.push_back({
            p.name, p.enabled,
            p.currentMode ? p.currentMode->size() : SkISize::MakeEmpty(),
            p.currentMode ? p.currentMode->refreshRate() : 0,
            p.pos, p.transform, p.scale });
    }

    CZWeak<DisplaysPane> weak { this };

    const bool sent { m_manager.apply(m_pending, [weak](MOutputManager::Result result){
        if (!weak)
            return;

        switch (result)
        {
        case MOutputManager::Result::Succeeded:
            weak->askToKeep();
            break;
        case MOutputManager::Result::Failed:
            weak->showError("The display settings couldn't be applied.", "The displays didn't accept them, nothing changed.");
            weak->reload();
            break;
        case MOutputManager::Result::Cancelled:
            weak->showError("The display settings couldn't be applied.", "The displays changed meanwhile. Check the settings and try again.");
            weak->reload();
            break;
        }
    }) };

    if (!sent)
        showError("The display settings couldn't be applied.", "Output management isn't available.");
}

void DisplaysPane::askToKeep() noexcept
{
    m_secondsLeft = ConfirmSeconds;

    auto dialog { std::make_unique<MDialog>("Keep these display settings?",
        std::format("The previous settings will be restored in {} seconds.", m_secondsLeft)) };
    dialog->setIcon("monitor", AKTheme::SystemBlue);

    // Closing the window reverts too (Cancel)
    dialog->addButton("Revert", MDialog::Role::Cancel, [this]{
        revert();
        return true;
    });

    dialog->addButton("Keep Changes", MDialog::Role::Default, [this]{
        m_countdown.stop();
        m_previous.clear();
        return true;
    });

    openDialog(std::move(dialog));
    m_countdown.start(1000);
}

void DisplaysPane::revert() noexcept
{
    m_countdown.stop();

    if (m_previous.empty())
        return;

    std::unordered_map<MOutputHead*, MOutputHead::Config> configs;

    for (const auto &saved : m_previous)
    {
        MOutputHead *head { m_manager.find(saved.name) };

        if (!head)
            continue;

        MOutputHead::Config config { head->config() };
        config.enabled = saved.enabled;
        config.pos = saved.pos;
        config.transform = saved.transform;
        config.scale = saved.scale;

        // Its mode, found by size and refresh rate
        for (auto *mode : head->props().modes)
            if (mode->size() == saved.modeSize && mode->refreshRate() == saved.refreshRate)
                config.mode = mode;

        configs[head] = config;
    }

    CZWeak<DisplaysPane> weak { this };

    const bool sent { m_manager.apply(configs, [weak](MOutputManager::Result result){
        if (!weak)
            return;

        if (result == MOutputManager::Result::Succeeded)
            weak->m_previous.clear();
        else if (result == MOutputManager::Result::Cancelled)
            weak->m_revertPending = true; // Again once the new state is known (see onChanged)
        else
        {
            weak->m_previous.clear();
            weak->showError("The previous display settings couldn't be restored.", "Restore them by hand.");
        }
    }) };

    if (!sent)
        m_previous.clear();
}

void DisplaysPane::showError(const std::string &title, const std::string &message) noexcept
{
    auto dialog { std::make_unique<MDialog>(title, message) };
    dialog->setIcon("monitor", AKTheme::SystemBlue);
    dialog->addButton("OK", MDialog::Role::Default);
    openDialog(std::move(dialog));
}

void DisplaysPane::openDialog(std::unique_ptr<MDialog> dialog) noexcept
{
    auto *window { scene() ? dynamic_cast<MToplevel*>(scene()->window()) : nullptr };

    if (!window)
        return;

    if (m_dialog)
        m_dialog->close();

    m_dialog = std::move(dialog);
    m_dialog->open(*window);
}

void DisplaysPane::colorSchemeEvent(const CZColorSchemeEvent &e)
{
    Pane::colorSchemeEvent(e);
    updateColors();
}

void DisplaysPane::updateColors() noexcept
{
    unavailable.setTextColor(AKTheme::SecondaryLabel);
}
