#include <Panes/WiFiPane.h>
#include <CZ/AK/AKScene.h>
#include <CZ/AK/AKTheme.h>
#include <CZ/Core/Events/CZColorSchemeEvent.h>
#include <CZ/Core/Events/CZPointerButtonEvent.h>
#include <CZ/Marco/Roles/MToplevel.h>

using namespace CZ;

static constexpr UInt32 ScanIntervalMs { 20000 };

/* ---------------------------------------- NetworkRow ---------------------------------------- */

NetworkRow::NetworkRow(WiFiPane &pane, bool known, AKNode *parent) noexcept :
    AKFormRow("", parent),
    m_pane(pane),
    m_known(known)
{
    const SkRegion empty;
    check.setInputRegion(&empty);
    lock.setInputRegion(&empty);

    // The names line up whether a row has a check mark or not
    check.setVisible(known);
    more.setVisible(known);
    more.setOutlined(true);

    setClickable(true);
    onClicked.subscribe(this, [this](const auto &){ m_pane.join(network); });
    more.onClick.subscribe(this, [this](const CZPointerButtonEvent &e){ m_pane.openMenu(network, more, e); });

    if (!known)
        leading.layout().setWidth(15.f); // Like the known rows' check mark column

    updateIconColors();
}

void NetworkRow::set(const SVWiFiNetwork &n, bool active) noexcept
{
    network = n;
    title.setText(n.ssid);
    check.setOpacity(active ? 1.f : 0.f);
    lock.setVisible(n.secured);
    signal.setStrength(n.strength);
    setClickable(!active);
}

void NetworkRow::colorSchemeEvent(const CZColorSchemeEvent &e)
{
    AKFormRow::colorSchemeEvent(e);
    updateIconColors();
}

void NetworkRow::updateIconColors() noexcept
{
    const SkColor label { AKTheme::Label.resolve(colorScheme()) };
    check.setColor(label);
    lock.setColor(label);
}

/* ----------------------------------------- WiFiPane ----------------------------------------- */

WiFiPane::WiFiPane() noexcept : Pane("wifi", "Wi-Fi")
{
    const SkRegion empty;

    toggle.onToggled.subscribe(this, [this](bool on){ wifi->setEnabled(on); });

    // The network it's connected to: its name, status (and a warning if it's open)
    currentRow.title.setFontSize(13.f);
    statusLine.layout().setAlignItems(YGAlignCenter);
    statusLine.layout().setGap(YGGutterColumn, 5.f);
    statusDot.layout().setWidth(8.f);
    statusDot.layout().setHeight(8.f);
    statusText.setFontSize(11.f);
    warningLine.layout().setAlignItems(YGAlignCenter);
    warningLine.layout().setGap(YGGutterColumn, 5.f);
    warningText.setFontSize(11.f);

    for (AKNode *n : std::initializer_list<AKNode*>{ &statusLine, &statusDot, &statusText, &warningLine, &warningIcon, &warningText })
        n->setInputRegion(&empty);

    // A plain button (not the accent color), like macOS's
    detailsButton.setBackgroundColor(AKTheme::ControlColor);
    detailsButton.setContentColor(AKTheme::ControlText);
    detailsButton.onClick.subscribe(this, [this](const auto &){ showDetails(); });

    messageRow.title.setTextColor(AKTheme::SecondaryLabel);

    searching.setInputRegion(&empty);
    searching.layout().setAlignItems(YGAlignCenter);
    searching.layout().setJustifyContent(YGJustifyCenter);
    searching.layout().setGap(YGGutterColumn, 8.f);
    searching.layout().setPadding(YGEdgeVertical, 12.f);
    searchingText.setInputRegion(&empty);
    searchingText.setFontSize(13.f);
    searchingText.setTextColor(AKTheme::SecondaryLabel);

    wifi->onChanged.subscribe(this, [this]{ update(); });
    wifi->onJoinFailed.subscribe(this, [this](const std::string &ssid, bool wrongPassword){
        for (const auto &n : wifi->state().networks)
        {
            if (n.ssid != ssid)
                continue;

            if (!wrongPassword)
                break;

            // A saved password that no longer works: asked again (and saved as a new profile)
            if (n.known())
                wifi->forget(n);

            askPassword(n, true);
            return;
        }

        auto dialog { std::make_unique<MDialog>("Couldn't join the Wi-Fi network \"" + ssid + "\".", "Make sure it's in range and try again.") };
        dialog->setIcon("wifi", AKTheme::SystemBlue);
        dialog->addButton("OK", MDialog::Role::Default);
        openDialog(std::move(dialog));
    });

    m_scanTimer.setCallback([this](CZTimer *){
        wifi->scan();
        m_scanTimer.start(ScanIntervalMs);
    });

    updateColors();
    update();
}

void WiFiPane::paneShown() noexcept
{
    wifi->scan();
    m_scanTimer.start(ScanIntervalMs);
}

void WiFiPane::paneHidden() noexcept
{
    m_scanTimer.stop();
}

void WiFiPane::update() noexcept
{
    const SVWiFiState &s { wifi->state() };
    const bool on { s.available && s.enabled };

    toggle.setOn(on);
    toggle.setEnabled(s.available);

    // Not available / off (nothing until the first read)
    messageRow.setVisible(s.loaded && !on);
    messageRow.title.setText(!s.available ? "No Wi-Fi hardware was found." : "Wi-Fi is turned off.");

    // The network it's connected (or connecting) to
    const SVWiFiNetwork *active { on ? s.active() : nullptr };
    currentRow.setVisible(active != nullptr);

    if (active)
    {
        const bool connected { s.link == SVWiFiState::Link::Connected };
        currentRow.title.setText(active->ssid);
        statusText.setText(connected ? "Connected" : "Connecting…");
        statusDot.setBackgroundColor((connected ? AKTheme::SystemGreen : AKTheme::SystemOrange).resolve(colorScheme()));
        warningLine.setVisible(!active->secured);
        currentSignal.setStrength(active->strength);
    }

    // Known networks in range (the one connected to with a check mark), then the others
    std::vector<const SVWiFiNetwork*> knownNets, otherNets;

    if (on)
        for (const auto &n : s.networks)
            (n.known() ? knownNets : otherNets).push_back(&n);

    // Spinning only while shown
    const bool isSearching { wifi->searching() };
    searching.setVisible(isSearching);

    if (isSearching)
        searchingSpinner.start();
    else
        searchingSpinner.stop();

    syncRows(known, m_knownRows, knownNets, true);
    syncRows(others, m_otherRows, otherNets, false);
    known.setVisible(!knownNets.empty());
    others.setVisible(!otherNets.empty());
}

void WiFiPane::syncRows(AKFormSection &section, std::vector<std::unique_ptr<NetworkRow>> &rows,
                        const std::vector<const SVWiFiNetwork*> &networks, bool isKnown) noexcept
{
    // Reused in place (no flicker while signal strengths change)
    while (rows.size() > networks.size())
        rows.pop_back();

    while (rows.size() < networks.size())
        rows.emplace_back(std::make_unique<NetworkRow>(*this, isKnown, &section.card));

    const SVWiFiState &s { wifi->state() };

    for (size_t i = 0; i < networks.size(); i++)
        rows[i]->set(*networks[i], networks[i]->ssid == s.activeSsid);

    section.updateSeparators();
}

void WiFiPane::join(const SVWiFiNetwork &network) noexcept
{
    if (network.ssid == wifi->state().activeSsid)
        return;

    if (network.enterprise || network.wep)
    {
        auto dialog { std::make_unique<MDialog>("Can't join \"" + network.ssid + "\".",
            network.securityName() + " networks aren't supported yet.") };
        dialog->setIcon("wifi", AKTheme::SystemBlue);
        dialog->addButton("OK", MDialog::Role::Default);
        openDialog(std::move(dialog));
        return;
    }

    if (network.secured && !network.known())
        askPassword(network, false);
    else
        wifi->join(network);
}

void WiFiPane::askPassword(const SVWiFiNetwork &network, bool wrong) noexcept
{
    auto dialog { std::make_unique<MDialog>(
        "The Wi-Fi network \"" + network.ssid + "\" requires a " + (network.sae ? "WPA3" : "WPA2") + " password.",
        wrong ? "The password was incorrect. Try again." : "") };
    dialog->setIcon("wifi", AKTheme::SystemBlue);

    m_password = std::make_unique<AKTextInput>("Password", &dialog->content);
    m_password->setPasswordMode(true);
    m_password->setShowsVisibilityToggle(true);
    MDialog *raw { dialog.get() };

    dialog->addButton("Cancel", MDialog::Role::Cancel);
    AKButton &joinButton { dialog->addButton("Join", MDialog::Role::Default, [this, network]{
        // WPA passwords have 8 to 63 characters
        if (m_password->text().size() < 8)
            return false;

        wifi->join(network, m_password->text());
        return true;
    }) };

    joinButton.setEnabled(false);
    m_password->onTextChanged.subscribe(raw, [&joinButton](const std::string &text){ joinButton.setEnabled(text.size() >= 8); });
    m_password->onSubmit.subscribe(raw, [raw](const std::string &){ raw->accept(); });
    m_password->onCancel.subscribe(raw, [raw]{ raw->reject(); });

    // Typing right away
    raw->onMappedChanged.subscribe(raw, [this, raw]{
        if (raw->mapped() && m_password)
            m_password->focus();
    });

    openDialog(std::move(dialog));
}

void WiFiPane::openMenu(const SVWiFiNetwork &network, AKNode &anchor, const CZPointerButtonEvent &event) noexcept
{
    m_menu.clear();
    m_menu.addAction("Auto-Join", [this, network]{ wifi->setAutoJoin(network, !network.autoJoin); }).setChecked(network.autoJoin);
    m_menu.addDivider();
    m_menu.addAction("Forget This Network…", [this, network]{ confirmForget(network); });
    m_menu.popup(anchor, &event);
}

void WiFiPane::confirmForget(const SVWiFiNetwork &network) noexcept
{
    auto dialog { std::make_unique<MDialog>("Forget Wi-Fi network \"" + network.ssid + "\"?",
        "This computer will no longer join this Wi-Fi network automatically.") };
    dialog->setIcon("wifi", AKTheme::SystemBlue);
    dialog->addButton("Cancel", MDialog::Role::Cancel);
    dialog->addButton("Forget", MDialog::Role::Default, [this, network]{
        wifi->forget(network);
        return true;
    });
    openDialog(std::move(dialog));
}

void WiFiPane::showDetails() noexcept
{
    const SVWiFiState &s { wifi->state() };
    const SVWiFiNetwork *active { s.active() };

    if (!active)
        return;

    auto dialog { std::make_unique<MDialog>(active->ssid) };
    dialog->setIcon("wifi", AKTheme::SystemBlue);
    dialog->layout().setWidth(360.f);

    // Label / value rows
    MDialog &d { *dialog };
    auto &section { d.own(std::make_unique<AKFormSection>("", &d.content)) };
    const auto row = [&d, &section](const std::string &label, const std::string &value) {
        auto &r { d.own(std::make_unique<AKFormRow>(label, &section.card)) };
        // Selectable (and copied with Ctrl+C), not editable
        auto &v { d.own(std::make_unique<AKCoreTextEditor>(value.empty() ? "—" : value, &r.trailing)) };
        v.setFontSize(13.f);
        v.setTextColor(AKTheme::SecondaryLabel);
        v.setSelectionColor(AKTheme::SelectedTextBackground);
        v.setSelectable(!value.empty());
    };

    row("Status", s.link == SVWiFiState::Link::Connected ? "Connected" : "Connecting…");
    row("Signal", std::to_string(active->strength) + " %");
    row("Security", active->securityName());
    row("IP Address", s.ipAddress);
    row("Router", s.router);
    row("Hardware Address", s.hwAddress);
    section.updateSeparators();

    const SVWiFiNetwork network { *active };

    if (network.known())
        dialog->addButton("Forget This Network…", MDialog::Role::Normal, [this, network]{
            // After this dialog closes
            CZTimer::OneShot(0, [this, network](CZTimer*){ confirmForget(network); });
            return true;
        });

    dialog->addButton("OK", MDialog::Role::Default);
    openDialog(std::move(dialog));
}

void WiFiPane::openDialog(std::unique_ptr<MDialog> dialog) noexcept
{
    auto *window { scene() ? dynamic_cast<MToplevel*>(scene()->window()) : nullptr };

    if (!window)
        return;

    // One at a time (the previous one, if any, goes away)
    if (m_dialog)
        m_dialog->close();

    m_dialog = std::move(dialog);
    m_dialog->open(*window);
}

void WiFiPane::colorSchemeEvent(const CZColorSchemeEvent &e)
{
    Pane::colorSchemeEvent(e);
    updateColors();
}

void WiFiPane::updateColors() noexcept
{
    const CZColorScheme scheme { colorScheme() };
    statusText.setTextColor(AKTheme::SecondaryLabel);
    warningText.setTextColor(AKTheme::SecondaryLabel);
    warningIcon.setColor(AKTheme::SystemYellow.resolve(scheme));
    update();
}
