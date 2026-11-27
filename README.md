# Settings

<p align="left">
  <a href="https://github.com/CuarzoSoftware/Settings/blob/main/LICENSE">
    <img src="https://img.shields.io/badge/license-LGPLv2.1-blue.svg" alt="Settings is released under the LGPLv2.1 license." />
  </a>
  <a href="https://github.com/CuarzoSoftware/Settings">
    <img src="https://img.shields.io/badge/version-0.1.0-brightgreen" alt="Current Settings version." />
  </a>
</p>

> This project is part of the [Cuarzo Framework](https://github.com/CuarzoSoftware/Cuarzo). Follow the instructions there to build and install it.

System Settings for Cuarzo OS (`cz-settings`), built on Marco and Kay, like macOS's System
Settings: a window with a sidebar (search field, the user's picture and name, and the sections of
settings) and the selected section's pane next to it, with back and forward navigation.

### Panes

- **Wi-Fi** (NetworkManager): turning Wi-Fi on and off, the network it's connected to (status,
  security, details: IP address, router, hardware address), known networks (Auto-Join, forget)
  and the other networks in range (joining them, asking for their password).
- **Trackpad** (Crystals): tracking speed, secondary click (two fingers or the bottom right
  corner), tap to click, natural scrolling, and the three finger swipes (between desktops, Exposé
  and Show Desktop). Saved to `~/.config/cz-crystals/trackpad.conf`, which Crystals applies right away.

The other sections show a placeholder for now.

### Requirements

- NetworkManager (system D-Bus) for the Wi-Fi pane, and `nmcli` to change Auto-Join.
