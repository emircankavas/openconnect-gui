# OpenConnect GUI (Multi-Profile & Split-DNS Fork)

This project is an enhanced fork of the official [OpenConnect GUI](https://gui.openconnect-vpn.net/) client, designed to provide concurrent multi-VPN connections, modern macOS Split-DNS management, and seamless credential persistence.

---

## Key Features in this Fork

### 1. Concurrent Multi-Profile VPN Connections & Modern Dashboard
- **Unified Dashboard UI:** Modern dark dashboard displaying all VPN profiles as interactive cards with instant toggle switches, protocol/cipher badges, DNS info, and real-time traffic statistics.
- **Simultaneous Connections:** Connect to and manage multiple enterprise VPN profiles at the same time directly from the dashboard without needing multiple windows.
- **Per-Profile State & Statistics:** Live status indicators, dynamic bandwidth metering (download/upload byte counters), and isolated connect/disconnect controls per profile card.
- **Tagged Activity Logs:** Progress and diagnostic logs are automatically prefixed with the active profile name (e.g., `[Office] CSTP connected...`), keeping logs clean and readable.
- **Single Window Mode:** Multi-window mode replaced with a unified card-based dashboard and strictly enforced single-instance mode.

### 2. Modern macOS Split-DNS & Wi-Fi Protection
- **Wi-Fi DNS Preservation:** Prevents VPN connections from overwriting the physical Wi-Fi/Ethernet interface's DHCP DNS servers via legacy `networksetup` overrides.
- **Dynamic SupplementalMatchDomains:** Leverages macOS native `scutil` `SupplementalMatchDomains` per `utun` interface. Queries matching the VPN's domains are routed to the VPN DNS, while local Wi-Fi and general internet queries remain untouched.
- **Guaranteed Cleanup:** Automatic removal of `scutil` resolver dictionaries upon profile disconnection and startup sweeping of any orphaned tunnel resolvers from unexpected system restarts.

### 3. Customizable Split-DNS Domains per Profile
- **GUI Configuration:** Directly define comma- or space-separated match domains (e.g., `company.com, company.com.tr`) under **Edit Profile -> Split DNS Domains**.
- Any traffic and queries to `*.company.com` automatically resolve through that profile's internal DNS servers.

### 4. Password Saving & Management
- **Profile Password Storage:** Easily set or update passwords directly in the **Edit Profile** dialog, with a toggleable Show/Hide button.
- **Encrypted Persistence:** Passwords are encrypted locally (`CryptData`) when the "Save password" checkbox is enabled.
- **Auto-Remember on Connect:** Passwords entered during the initial connection prompt are remembered automatically for future one-click logins.

---

## Supported Platforms
- macOS 12.0 and newer (Apple Silicon ARM64 & Intel x86_64)
- Microsoft Windows 10 and newer

---

## Building from Source (macOS)

### Prerequisites (via Homebrew)
```bash
brew install qt@6 openconnect gnutls spdlog fmt cmake
```

### Build & Run
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
cmake --build build --config Release

# Run with required network privileges:
sudo ./build/bin/OpenConnect-GUI.app/Contents/MacOS/OpenConnect-GUI
```

---

## Upstream Project & Documentation
- Original Project: [OpenConnect GUI](https://gui.openconnect-vpn.net/)
- Developer Documentation: [Compilation](docs/dev.md) | [Development with QtCreator](docs/dev_QtCreator.md)

---

# License
This project is licensed under the [GNU General Public License v2](LICENSE.txt).
