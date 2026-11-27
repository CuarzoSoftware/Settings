#include <Window/SettingsWindow.h>
#include <Panes/WiFiPane.h>
#include <Panes/DisplaysPane.h>
#include <Panes/SoundPane.h>
#include <Panes/KeyboardPane.h>
#include <Panes/TrackpadPane.h>
#include <Panes/PlaceholderPane.h>
#include <CZ/AK/AKTheme.h>
#include <CZ/Core/CZTimer.h>
#include <cstdio>

using namespace CZ;

SettingsWindow::SettingsWindow() noexcept
{
    setTitle("System Settings");
    setColor(CZAdaptiveColor(0)); // Transparent: the sidebar shows the background blur
    layout().setFlexDirection(YGFlexDirectionRow);
    sidebar.setWidth(SidebarWidth);
    layout().setWidth(DefaultWidth);
    layout().setHeight(DefaultHeight);
    setMinSize(SkISize::Make(Int32(sidebar.minWidth()) + MinPaneWidth, MinHeight));

    sidebar.list.onSelectionChanged.subscribe(this, [this](AKSidebarItem *item){
        if (item)
            present(item->id);
    });

    paneArea.back.onClick.subscribe(this, [this](const auto &){ goBack(); });
    paneArea.forward.onClick.subscribe(this, [this](const auto &){ goForward(); });

    show("wifi");
}

SettingsWindow::PaneArea::PaneArea(AKNode *parent) noexcept : AKSolidColor(AKTheme::WindowBackground, parent)
{
    layout().setFlex(1.f);
    layout().setMinWidth(MinPaneWidth);
    layout().setHeightPercent(100.f);
    enableChildrenClipping(false); // The separator is drawn just outside its left edge

    toolbar.userCaps.add(UCWindowMove);
    toolbar.layout().setHeight(MSidebar::HeaderHeight);
    toolbar.layout().setFlexShrink(0.f);
    toolbar.layout().setAlignItems(YGAlignCenter);
    toolbar.layout().setPadding(YGEdgeHorizontal, 12.f);
    toolbar.layout().setGap(YGGutterColumn, 4.f);

    title.setFontSize(15.f);
    title.setFontStyle(SkFontStyle(SkFontStyle::kBold_Weight, SkFontStyle::kNormal_Width, SkFontStyle::kUpright_Slant));
    title.setTextColor(AKTheme::Label);
    title.layout().setMargin(YGEdgeLeft, 8.f);
    const SkRegion empty;
    title.setInputRegion(&empty);

    scroll.layout().setFlex(1.f);
    scroll.slot()->layout().setPadding(YGEdgeHorizontal, PanePadding);
    scroll.slot()->layout().setPadding(YGEdgeBottom, PanePadding);
    scroll.slot()->layout().setPadding(YGEdgeTop, 6.f);

    separator.enableDiminishOpacityOnInactive(true);

    toolbarShadowArea.setInputRegion(&empty);
    toolbarShadowArea.layout().setPositionType(YGPositionTypeAbsolute);
    toolbarShadowArea.layout().setPosition(YGEdgeTop, 0.f);
    toolbarShadowArea.layout().setPosition(YGEdgeLeft, 0.f);
    toolbarShadowArea.layout().setWidthPercent(100.f);
    toolbarShadowArea.layout().setHeight(MSidebar::HeaderHeight);
    toolbarShadow.setStrength(0.6f);
    toolbarShadow.showWhileScrolled(&scroll);
}

Pane *SettingsWindow::pane(const std::string &id) noexcept
{
    if (auto it { m_panes.find(id) }; it != m_panes.end())
        return it->second.get();

    std::unique_ptr<Pane> pane;

    if (id == "wifi")
        pane = std::make_unique<WiFiPane>();
    else if (id == "displays")
        pane = std::make_unique<DisplaysPane>();
    else if (id == "sound")
        pane = std::make_unique<SoundPane>();
    else if (id == "keyboard")
        pane = std::make_unique<KeyboardPane>();
    else if (id == "trackpad")
        pane = std::make_unique<TrackpadPane>();
    else if (AKSidebarItem *item { sidebar.list.item(id) })
        pane = std::make_unique<PlaceholderPane>(id, item->label.text(), item->tile.icon.icon(), item->tileColor());
    else
        return nullptr;

    return (m_panes[id] = std::move(pane)).get();
}

void SettingsWindow::show(const std::string &id) noexcept
{
    // Selecting it presents it (see the constructor)
    if (AKSidebarItem *item { sidebar.list.item(id) })
    {
        if (sidebar.list.selectedItem() == item)
            present(id);
        else
            sidebar.list.setSelectedItem(item);
    }
}

void SettingsWindow::present(const std::string &id) noexcept
{
    Pane *next { pane(id) };

    if (!next || next == m_current)
        return;

    if (m_current)
    {
        if (!m_navigating)
        {
            m_backStack.push_back(m_current->id);
            m_forwardStack.clear();
        }

        m_current->paneHidden();
        m_current->setParent(nullptr);
    }

    m_current = next;
    m_current->setParent(&paneArea.scroll);
    paneArea.scroll.setOffset(0.f, 0.f);
    paneArea.title.setText(m_current->title);
    m_current->paneShown();
    updateNavigation();
}

void SettingsWindow::goBack() noexcept
{
    if (m_backStack.empty() || !m_current)
        return;

    const std::string id { m_backStack.back() };
    m_backStack.pop_back();
    m_forwardStack.push_back(m_current->id);
    m_navigating = true;
    show(id);
    m_navigating = false;
    updateNavigation();
}

void SettingsWindow::goForward() noexcept
{
    if (m_forwardStack.empty() || !m_current)
        return;

    const std::string id { m_forwardStack.back() };
    m_forwardStack.pop_back();
    m_backStack.push_back(m_current->id);
    m_navigating = true;
    show(id);
    m_navigating = false;
    updateNavigation();
}

void SettingsWindow::updateNavigation() noexcept
{
    paneArea.back.setEnabled(!m_backStack.empty());
    paneArea.forward.setEnabled(!m_forwardStack.empty());
}
