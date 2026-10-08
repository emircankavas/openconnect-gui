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

#include "ConsoleUi.h"

#include "common.h"
#include "logger.h"
#include "posixcompat.h"

#include <QTextStream>

#include <cstdio>
#include <cstring>
#include <string>

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <termios.h>
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

static QTextStream& outStream()
{
    static QTextStream s(stdout);
    return s;
}

static QTextStream& errStream()
{
    static QTextStream s(stderr);
    return s;
}

static bool stdinIsTty()
{
    return ISATTY(FILENO(stdin)) != 0;
}

// Read a line from the terminal without echoing it (passwords / PINs).
bool ConsoleUi::readSecret(QString& out)
{
#ifdef _WIN32
    out.clear();
    int c;
    for (;;) {
        c = _getch();
        if (c == '\r' || c == '\n') {
            break;
        }
        if (c == 0 || c == 0xE0) { // function key prefix
            _getch();
            continue;
        }
        if (c == 8) { // backspace
            if (!out.isEmpty()) {
                out.chop(1);
                fputs("\b \b", stdout);
                fflush(stdout);
            }
            continue;
        }
        if (c == 3) { // Ctrl-C
            return false;
        }
        out.append(QChar(c));
        fputc('*', stdout);
        fflush(stdout);
    }
    fputc('\n', stdout);
    fflush(stdout);
    return true;
#else
    struct termios oldt, newt;
    if (tcgetattr(STDIN_FILENO, &oldt) != 0) {
        return false;
    }
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::string line;
    char ch;
    bool ok = true;
    while (true) {
        int n = ocg_read(OCG_STDIN_FILENO, &ch, 1);
        if (n <= 0) {
            ok = false;
            break;
        }
        if (ch == '\n' || ch == '\r') {
            break;
        }
        if (ch == 3) { // Ctrl-C
            ok = false;
            break;
        }
        line.push_back(ch);
    }
    if (ch != '\n') {
        ocg_write(OCG_STDOUT_FILENO, "\n", 1);
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    out = QString::fromUtf8(line.c_str());
    return ok;
#endif
}

bool ConsoleUi::promptText(const QString& title, const QString& label, bool secret,
    const QString& banner, const QString& message, QString& out)
{
    // Values supplied on the command line take precedence (once).
    if (secret && !m_passwordUsed && !m_password.isEmpty()) {
        out = m_password;
        m_passwordUsed = true;
        return true;
    }
    if (!secret && !m_usernameUsed && !m_username.isEmpty()) {
        out = m_username;
        m_usernameUsed = true;
        return true;
    }

    if (!banner.isEmpty()) {
        errStream() << banner << '\n';
    }
    if (!message.isEmpty()) {
        errStream() << message << '\n';
    }

    if (!m_interactive) {
        errStream() << "error: " << title << ": no value supplied and stdin is not a TTY\n";
        return false;
    }

    errStream() << label << (secret ? ": " : " []: ");
    errStream().flush();

    if (secret) {
        return readSecret(out);
    }

    std::string line;
    char ch;
    bool ok = true;
    while (true) {
        int n = ocg_read(OCG_STDIN_FILENO, &ch, 1);
        if (n <= 0) { ok = false; break; }
        if (ch == '\n' || ch == '\r') { break; }
        line.push_back(ch);
    }
    out = QString::fromUtf8(line.c_str());
    return ok;
}

bool ConsoleUi::promptSelect(const QString& title, const QString& label,
    const QStringList& options, const QString& banner, const QString& message, QString& out)
{
    if (!m_groupUsed && !m_group.isEmpty() && options.contains(m_group)) {
        out = m_group;
        m_groupUsed = true;
        return true;
    }

    if (!banner.isEmpty()) {
        errStream() << banner << '\n';
    }
    if (!message.isEmpty()) {
        errStream() << message << '\n';
    }

    if (!m_interactive) {
        errStream() << "error: " << title << ": no value supplied and stdin is not a TTY\n";
        return false;
    }

    errStream() << label << ":\n";
    for (int i = 0; i < options.size(); ++i) {
        errStream() << "  " << (i + 1) << ") " << options.at(i) << '\n';
    }
    errStream() << "Select [1-" << options.size() << "]: ";
    errStream().flush();

    std::string line;
    char ch;
    while (true) {
        int n = ocg_read(OCG_STDIN_FILENO, &ch, 1);
        if (n <= 0) { return false; }
        if (ch == '\n' || ch == '\r') { break; }
        line.push_back(ch);
    }

    QString sel = QString::fromUtf8(line.c_str()).trimmed();
    // accept either the number or the exact option label
    bool ok = false;
    int idx = sel.toInt(&ok);
    if (ok && idx >= 1 && idx <= options.size()) {
        out = options.at(idx - 1);
        return true;
    }
    if (options.contains(sel)) {
        out = sel;
        return true;
    }
    return false;
}

bool ConsoleUi::confirmPeerCert(const QString& text, const QString& hostInfo,
    const QString& details, const QString& acceptText)
{
    errStream() << text << '\n';
    errStream() << hostInfo << '\n';
    if (!details.isEmpty()) {
        errStream() << details << '\n';
    }
    errStream().flush();

    // Headless safe default: refuse an unknown/changed certificate unless
    // --trust-tofu was given. This never auto-accepts silently.
    if (!m_trustTofu) {
        errStream() << "error: server certificate is not trusted; "
                       "re-run with --trust-tofu to accept and pin it\n";
        return false;
    }

    if (!m_interactive) {
        errStream() << "trusting (--trust-tofu) and pinning the certificate\n";
        return true;
    }

    errStream() << acceptText << "? [y/N]: ";
    errStream().flush();
    char ch = 0;
    if (ocg_read(OCG_STDIN_FILENO, &ch, 1) <= 0) {
        return false;
    }
    bool accept = (ch == 'y' || ch == 'Y');
    // drain to end of line
    char skip = ch;
    while (skip != '\n' && skip != '\r' && skip != 0) {
        if (ocg_read(OCG_STDIN_FILENO, &skip, 1) <= 0) break;
    }
    return accept;
}

bool ConsoleUi::confirmBanner(const QString& banner)
{
    errStream() << banner << '\n' << "Accept? [y/N]: ";
    errStream().flush();

    if (!m_interactive) {
        // non-interactive sessions carry on (matching batch mode)
        return true;
    }

    char ch = 0;
    if (ocg_read(OCG_STDIN_FILENO, &ch, 1) <= 0) {
        return false;
    }
    bool accept = (ch == 'y' || ch == 'Y');
    char skip = ch;
    while (skip != '\n' && skip != '\r' && skip != 0) {
        if (ocg_read(OCG_STDIN_FILENO, &skip, 1) <= 0) break;
    }
    return accept;
}

QString ConsoleUi::pinPrompt(const QString& tokenUrl, const QString& label, unsigned flags)
{
    if (!m_pin.isEmpty()) {
        QString pin = m_pin;
        m_pin.clear();
        return pin;
    }

    QString type = QStringLiteral("user");
    if (flags & GNUTLS_PIN_SO) {
        type = QStringLiteral("security officer");
    }

    QString text = QStringLiteral("Please enter the %1 PIN for %2.")
                       .arg(type)
                       .arg(label);
    errStream() << text << ' ';
    errStream().flush();

    if (!m_interactive) {
        return QString();
    }

    QString pin;
    if (!readSecret(pin)) {
        return QString();
    }
    return pin;
}

void ConsoleUi::onStats(const QString& profileName, const struct oc_stats* stats,
    const QString& dtls)
{
    Q_UNUSED(profileName);
    Q_UNUSED(dtls);
    if (stats == nullptr) {
        return;
    }
    outStream() << "stats: rx=" << stats->rx_bytes << " tx=" << stats->tx_bytes
                << '\n';
    outStream().flush();
}

void ConsoleUi::onStatus(const QString& profileName, status_t status,
    const QString& dns, const QString& ip, const QString& ip6,
    const QString& cstp, const QString& dtls)
{
    Q_UNUSED(profileName);
    switch (status) {
    case STATUS_CONNECTING:
        outStream() << "connecting\n";
        break;
    case STATUS_CONNECTED:
        outStream() << "connected ip=" << ip;
        if (!ip6.isEmpty()) {
            outStream() << " ip6=" << ip6;
        }
        if (!dns.isEmpty()) {
            outStream() << " dns=" << dns;
        }
        if (!cstp.isEmpty()) {
            outStream() << " cstp=" << cstp;
        }
        if (!dtls.isEmpty()) {
            outStream() << " dtls=" << dtls;
        }
        outStream() << '\n';
        break;
    case STATUS_DISCONNECTING:
        outStream() << "disconnecting\n";
        break;
    case STATUS_DISCONNECTED:
        outStream() << "disconnected\n";
        break;
    }
    outStream().flush();
}

void ConsoleUi::requestDisconnect(const QString& profileName)
{
    Q_UNUSED(profileName);
    m_disconnectRequested = true;
}
