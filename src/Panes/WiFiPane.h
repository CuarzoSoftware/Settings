#ifndef WIFIPANE_H
#define WIFIPANE_H

#include <Panes/Pane.h>
#include <CZ/Services/SVWiFi.h>
#include <CZ/AK/Nodes/AKFormSection.h>
#include <CZ/AK/Nodes/AKFormRow.h>
#include <CZ/AK/Nodes/AKIconTile.h>
#include <CZ/AK/Nodes/AKIconButton.h>
#include <CZ/AK/Nodes/AKSwitch.h>
#include <CZ/AK/Nodes/AKButton.h>
#include <CZ/AK/Nodes/AKFontIcon.h>
#include <CZ/AK/Nodes/AKWiFiIcon.h>
#include <CZ/AK/Nodes/AKActivityIndicator.h>
#include <CZ/AK/Nodes/AKRoundSolidColor.h>
#include <CZ/AK/Nodes/AKTextInput.h>
#include <CZ/AK/Nodes/AKCoreTextEditor.h>
#include <CZ/Marco/Roles/MMenu.h>
#include <CZ/Marco/Roles/MDialog.h>
#include <CZ/Core/CZTimer.h>
#include <CZ/AK/AKTheme.h>
#include <memory>
#include <vector>

class WiFiPane;

/// A network of the Known Networks or Other Networks section.
class NetworkRow final : public CZ::AKFormRow
{
public:
    NetworkRow(WiFiPane &pane, bool known, CZ::AKNode *parent) noexcept;

    void set(const CZ::SVWiFiNetwork &network, bool active) noexcept;

    CZ::SVWiFiNetwork network;
    CZ::AKFontIcon check { "check", 15, &leading };   // Known: the one connected to
    CZ::AKFontIcon lock { "lock", 14, &trailing };
    CZ::AKWiFiIcon signal { 100, 17, &trailing };
    CZ::AKIconButton more { "more_horiz", 20, &trailing }; // Known: its menu

protected:
    void colorSchemeEvent(const CZ::CZColorSchemeEvent &e) override;

private:
    void updateIconColors() noexcept;
    WiFiPane &m_pane;
    bool m_known;
};

/// Wi-Fi: turning it on and off, the network it's connected to and the ones in range.
class WiFiPane final : public Pane
{
public:
    WiFiPane() noexcept;

    void paneShown() noexcept override;
    void paneHidden() noexcept override;

    /// Joins @p network (asking for its password if needed).
    void join(const CZ::SVWiFiNetwork &network) noexcept;

    /// Its menu (Auto-Join, Forget...), under @p anchor.
    void openMenu(const CZ::SVWiFiNetwork &network, CZ::AKNode &anchor, const CZ::CZPointerButtonEvent &event) noexcept;

    void showDetails() noexcept;
    void confirmForget(const CZ::SVWiFiNetwork &network) noexcept;

    std::shared_ptr<CZ::SVWiFi> wifi { CZ::SVWiFi::GetOrMake() };

    // Wi-Fi on/off, and the network it's connected to
    CZ::AKFormSection main { "", this };
    CZ::AKFormRow toggleRow { "Wi-Fi", &main.card };
    CZ::AKIconTile toggleIcon { "wifi", CZ::AKTheme::SystemBlue, 26, &toggleRow.leading };
    CZ::AKSwitch toggle { false, &toggleRow.trailing };

    CZ::AKFormRow currentRow { "", &main.card };
    CZ::AKContainer statusLine { YGFlexDirectionRow, false, &currentRow.texts };
    CZ::AKRoundSolidColor statusDot { 0, CZ::CZBorderRadius::Make(4), &statusLine };
    CZ::AKText statusText { "", &statusLine };
    CZ::AKContainer warningLine { YGFlexDirectionRow, false, &currentRow.texts };
    CZ::AKFontIcon warningIcon { "warning", 13, &warningLine };
    CZ::AKText warningText { "Unsecured Network", &warningLine };
    CZ::AKWiFiIcon currentSignal { 100, 17, &currentRow.trailing };
    CZ::AKButton detailsButton { "Details…", &currentRow.trailing };

    CZ::AKFormRow messageRow { "", &main.card }; // No hardware / Wi-Fi off

    // On, but no network listed yet
    CZ::AKContainer searching { YGFlexDirectionRow, false, this };
    CZ::AKActivityIndicator searchingSpinner { 16, &searching };
    CZ::AKText searchingText { "Looking for networks…", &searching };

    CZ::AKFormSection known { "Known Networks", this };
    CZ::AKFormSection others { "Other Networks", this };

protected:
    void colorSchemeEvent(const CZ::CZColorSchemeEvent &e) override;

private:
    void update() noexcept;
    void syncRows(CZ::AKFormSection &section, std::vector<std::unique_ptr<NetworkRow>> &rows,
                  const std::vector<const CZ::SVWiFiNetwork*> &networks, bool known) noexcept;
    void askPassword(const CZ::SVWiFiNetwork &network, bool wrong) noexcept;
    void openDialog(std::unique_ptr<CZ::MDialog> dialog) noexcept;
    void updateColors() noexcept;

    std::vector<std::unique_ptr<NetworkRow>> m_knownRows, m_otherRows;
    std::unique_ptr<CZ::MDialog> m_dialog;
    std::unique_ptr<CZ::AKTextInput> m_password; // In the password dialog
    CZ::MMenu m_menu;
    CZ::CZTimer m_scanTimer;
};

#endif // WIFIPANE_H
