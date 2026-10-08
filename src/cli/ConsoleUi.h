/*
 * Headless VpnUi implementation for ocg-cli.
 *
 * This file is part of openconnect-gui.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QString>
#include <QStringList>

#include "vpnui.h"

class ConsoleUi : public VpnUi {
public:
    // When non-interactive (no TTY), prompts are not attempted; values must
    // come from the profile or command-line options.
    explicit ConsoleUi(bool interactive, bool trustTofu)
        : m_interactive(interactive)
        , m_trustTofu(trustTofu)
    {
    }

    // Values that may be supplied by command-line options instead of a prompt.
    void setUsername(const QString& v) { m_username = v; }
    void setPassword(const QString& v) { m_password = v; }
    void setGroup(const QString& v) { m_group = v; }
    void setPin(const QString& v) { m_pin = v; }

    bool promptText(const QString& title, const QString& label, bool secret,
        const QString& banner, const QString& message, QString& out) override;
    bool promptSelect(const QString& title, const QString& label,
        const QStringList& options, const QString& banner, const QString& message,
        QString& out) override;
    bool confirmPeerCert(const QString& text, const QString& hostInfo,
        const QString& details, const QString& acceptText) override;
    bool confirmBanner(const QString& banner) override;
    QString pinPrompt(const QString& tokenUrl, const QString& label, unsigned flags) override;
    void onStats(const QString& profileName, const struct oc_stats* stats,
        const QString& dtls) override;
    void onStatus(const QString& profileName, status_t status,
        const QString& dns, const QString& ip, const QString& ip6,
        const QString& cstp, const QString& dtls) override;
    void requestDisconnect(const QString& profileName) override;

    bool disconnectedRequested() const { return m_disconnectRequested; }

private:
    bool readSecret(QString& out);

    bool m_interactive;
    bool m_trustTofu;
    bool m_disconnectRequested = false;

    // consumed-once values set from the command line
    QString m_username;
    QString m_password;
    QString m_group;
    QString m_pin;

    // once the user supplies a value through a prompt it is remembered for
    // subsequent prompts of the same kind within the same session
    bool m_usernameUsed = false;
    bool m_passwordUsed = false;
    bool m_groupUsed = false;
};
