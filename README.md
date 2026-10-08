# OpenConnect GUI (Multi-Profile, Split-DNS & Headless CLI Fork)

This project is an enhanced fork of the official [OpenConnect GUI](https://gui.openconnect-vpn.net/) client, designed to provide concurrent multi-VPN connections, modern Split-DNS management, per-profile customization, and a headless command-line client for servers.

---

## Key Features in this Fork

### 1. Concurrent Multi-Profile VPN Connections & Modern Dashboard
- **Unified Dashboard UI:** Modern dark dashboard displaying all VPN profiles as interactive cards with instant toggle switches, protocol/cipher badges, DNS info, and real-time traffic statistics.
- **Simultaneous Connections:** Connect to and manage multiple enterprise VPN profiles at the same time directly from the dashboard without needing multiple windows.
- **Per-Profile State & Statistics:** Live status indicators, dynamic bandwidth metering (download/upload byte counters), and isolated connect/disconnect controls per profile card.
- **Tagged Activity Logs:** Progress and diagnostic logs are automatically prefixed with the active profile name (e.g., `[Office] CSTP connected...`), keeping logs clean and readable.
- **Single Window Mode:** Multi-window mode replaced with a unified card-based dashboard and strictly enforced single-instance mode.

### 2. Split-DNS with Wi-Fi DNS Preservation
Per-profile split-DNS is set up natively on each platform, so queries for the VPN's domains are answered by the VPN DNS while local Wi-Fi and general internet resolution stay untouched.
- **macOS:** native `scutil` `SupplementalMatchDomains` per `utun` interface. The physical Wi-Fi/Ethernet interface's DHCP DNS servers are never overwritten by legacy `networksetup` overrides. Resolver dictionaries are removed on disconnect and orphaned ones are swept on startup.
- **Linux:** `systemd-resolved` (`resolvectl`) routing-only domains (`~domain`) when available, otherwise appended to `/etc/resolv.conf`.
- **Windows:** routing and DNS are handled by the bundled `vpnc-script.js` through the Wintun adapter.

### 3. Customizable Split-DNS Domains per Profile
- **GUI Configuration:** Directly define comma- or space-separated match domains (e.g., `company.com, company.com.tr`) under **Edit Profile -> Split DNS Domains**.
- Any traffic and queries to `*.company.com` automatically resolve through that profile's internal DNS servers.

### 4. Password Saving & Management
- **Profile Password Storage:** Easily set or update passwords directly in the **Edit Profile** dialog, with a toggleable Show/Hide button.
- **Encrypted Persistence:** Passwords are encrypted locally with the Windows Data Protection API (DPAPI) on Windows (`CryptData`).
- **Auto-Remember on Connect:** Passwords entered during the initial connection prompt are remembered automatically for future one-click logins.

### 5. Headless CLI (`ocg-cli`)
A separate, GUI-less front-end that shares the same connection core — useful on Linux servers with no graphical session.
- **Commands:** `list` · `connect <profile>` · `disconnect [profile]` · `status [profile]`
- **Options:** `--username`, `--password-file <file|->`, `--group`, `--pin`, `--trust-tofu`, `--foreground`, `--log-level <err|info|debug|trace>`, `--json`
- **Background by default:** `connect` daemonizes (POSIX `fork` + readiness pipe) and writes a per-profile state file used by `disconnect`/`status`.
- **Safe by default:** an unknown server certificate is refused unless `--trust-tofu` is given; secrets are read from a file or stdin, never from the command line.

---

## Supported Platforms
- **macOS** 12.0 and newer (Apple Silicon ARM64 & Intel x86_64)
- **Windows** 10 and newer (MinGW/MSYS2 build, NSIS installer)
- **Linux** (x86_64) — packaged as a `.deb` (Qt 6 + libopenconnect from the distro)

CI (GitHub Actions) builds all three on every push to `main` and publishes the
artifacts to a rolling `continuous` pre-release:
- `OpenConnect-GUI-macos-arm64.zip`
- `openconnect-gui-<ver>-oc-<ocver>-win64.exe` (installer)
- `openconnect-gui_<ver>_amd64.deb`

---

## Installing

### macOS
Download the `.zip`, unzip, clear the quarantine flag (unsigned build) and run
with network privileges:
```bash
unzip OpenConnect-GUI-macos-arm64.zip
xattr -dr com.apple.quarantine OpenConnect-GUI.app
sudo OpenConnect-GUI.app/Contents/MacOS/OpenConnect-GUI
```

### Windows
Run the `openconnect-gui-*-win64.exe` installer. It bundles openconnect
(≥ 9.21, required for the STRAP/TLSv1.3 fix on newer Cisco ASA/FTD gateways),
Qt 6, GnuTLS and the Wintun driver. The application requests administrator
rights via UAC.

### Linux
```bash
sudo apt install ./openconnect-gui_<ver>_amd64.deb
```
The package installs both `openconnect-gui` (GUI) and `ocg-cli` (headless),
the bundled `vpnc-script` (split-DNS), a desktop entry and an icon. It depends
only on Qt 6, `libopenconnect5` and GnuTLS — `fmt`/`spdlog` are embedded — so
the same `.deb` installs across Ubuntu releases.

CLI quick start:
```bash
ocg-cli list
sudo ocg-cli connect "Office"          # daemonizes
ocg-cli status
sudo ocg-cli disconnect "Office"
```

---

## Building from Source

### macOS
```bash
brew install qt@6 openconnect gnutls spdlog fmt cmake
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
cmake --build build --config Release
sudo ./build/bin/OpenConnect-GUI.app/Contents/MacOS/OpenConnect-GUI
```

### Linux (Debian/Ubuntu)
```bash
sudo apt install build-essential cmake ninja-build pkg-config \
    qt6-base-dev qt6-base-dev-tools qt6-scxml-dev \
    libopenconnect-dev libgnutls28-dev libspdlog-dev libfmt-dev
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel "$(nproc)"
cd build && cpack -G DEB -C Release      # -> openconnect-gui_<ver>_amd64.deb
```

### Windows (MSYS2 / MinGW-w64)
Two helper scripts drive the whole dependency + application build:
```bash
./contrib/build_deps_mingw@msys2.sh      # builds openconnect v9.21 + deps
./contrib/build_mingw@msys2.sh           # builds the app and the NSIS package
```
The binaries end up in `build/bin/` and the installer
`openconnect-gui-*-win64.exe` in the build directory.

---

## Upstream Project & Documentation
- Original Project: [OpenConnect GUI](https://gui.openconnect-vpn.net/)
- Developer Documentation: [Compilation](docs/dev.md) | [Development with QtCreator](docs/dev_QtCreator.md)

---

# License
This project is licensed under the [GNU General Public License v2](LICENSE.txt).
