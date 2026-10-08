/*
 * Copyright (C) 2014 Red Hat
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

// The presentation interface used by the connection core (VpnInfo). It lets
// the same auth/certificate/statistics logic drive either the Qt Widgets GUI
// (MainWindow) or a headless console front-end (ocg-cli). It deliberately has
// no dependency on QtWidgets so the CLI can link only Qt6::Core.

#include <QString>
#include <QStringList>

struct oc_stats;

enum status_t {
    STATUS_DISCONNECTING,
    STATUS_DISCONNECTED,
    STATUS_CONNECTING,
    STATUS_CONNECTED
};

class VpnUi {
public:
    virtual ~VpnUi() = default;

    // Free text or password (secret=true) input. Returns false when the user
    // cancels. `banner`/`message` carry any server-provided context.
    virtual bool promptText(const QString& title, const QString& label, bool secret,
                            const QString& banner, const QString& message, QString& out)
        = 0;

    // Choose one of `options`. Returns false on cancel.
    virtual bool promptSelect(const QString& title, const QString& label,
                              const QStringList& options,
                              const QString& banner, const QString& message, QString& out)
        = 0;

    // Peer certificate trust prompt (trust-on-first-use / key change). Return
    // true to accept and (for an unknown key) pin it.
    virtual bool confirmPeerCert(const QString& text, const QString& hostInfo,
                                 const QString& details, const QString& acceptText)
        = 0;

    // Informational banner from the server; return false to abort the session.
    virtual bool confirmBanner(const QString& banner) = 0;

    // PKCS#11 PIN. Return an empty string to abort.
    virtual QString pinPrompt(const QString& tokenUrl, const QString& label,
                              unsigned flags)
        = 0;

    // Live statistics update.
    virtual void onStats(const QString& profileName, const struct oc_stats* stats,
                         const QString& dtls)
        = 0;

    // Connection lifecycle notification.
    virtual void onStatus(const QString& profileName, status_t status,
                          const QString& dns, const QString& ip, const QString& ip6,
                          const QString& cstp, const QString& dtls)
        = 0;

    // Request that an in-progress session be torn down (e.g. the user declined
    // the server banner). The front-end decides how to stop it.
    virtual void requestDisconnect(const QString& profileName) = 0;
};
