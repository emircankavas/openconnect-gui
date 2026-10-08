/*
 * ocg-cli — headless front-end for OpenConnect-GUI.
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

#include "OcSettings.h"
#include "config.h"
#include "keyprompt.h"
#include "posixcompat.h"
#include "logger.h"
#include "server_storage.h"
#include "vpninfo.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStringList>
#include <QTextStream>
#include <QUrl>

#include <csignal>
#include <cstdio>

#ifndef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

static QTextStream& out() { static QTextStream s(stdout); return s; }
static QTextStream& err() { static QTextStream s(stderr); return s; }

// ---- tiny command-line parser ---------------------------------------------

struct Options {
    QString command;
    QString profile;
    QString username;
    QString password;      // from --password-file (or empty)
    QString group;
    QString pin;
    bool trustTofu = false;
    bool daemon = false;
    bool json = false;
    int logLevel = -1;
};

static void printUsage()
{
    out() << "Usage: ocg-cli <command> [options]\n\n"
             "Commands:\n"
             "  list                       List saved profiles\n"
             "  connect <profile>          Connect to a profile (daemonizes by default)\n"
             "  disconnect [profile]       Stop a running connection\n"
             "  status [profile]           Show connection status\n\n"
             "Options for connect:\n"
             "  --username <user>          VPN username\n"
             "  --password-file <file>     Read the password from a file ('-' = stdin)\n"
             "  --group <name>             Authentication group\n"
             "  --pin <pin>                PKCS#11 PIN\n"
             "  --trust-tofu               Accept and pin an unknown certificate\n"
             "  --foreground               Do not daemonize; run in the foreground\n"
             "  --log-level <err|info|debug|trace>\n"
             "Global:\n"
             "  --json                     Machine-readable output\n"
             "  -v, --version              Show version\n"
             "  -h, --help                 Show this help\n";
    out().flush();
}

// ---- profile helpers -------------------------------------------------------

static QStringList profileNames()
{
    QStringList names;
    OcSettings settings;
    const QString prefix = QStringLiteral("server:");
    for (const auto& key : settings.allKeys()) {
        if (key.startsWith(prefix) && key.endsWith(QStringLiteral("/server"))) {
            QString name = key;
            name.remove(0, prefix.size());
            name.chop(QStringLiteral("/server").size());
            names << name;
        }
    }
    names.sort();
    names.removeDuplicates();
    return names;
}

static QString stateDir()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (dir.isEmpty()) {
        dir = QDir::homePath() + QStringLiteral("/.local/share/openconnect-gui");
    }
    dir += QStringLiteral("/state");
    QDir().mkpath(dir);
    return dir;
}

static QString stateFileFor(const QString& profile)
{
    return stateDir() + QStringLiteral("/") + profile + QStringLiteral(".state");
}

// ---- signal handling (foreground/mainloop cancellation) --------------------

#ifndef _WIN32
static volatile sig_atomic_t g_stop = 0;
static int g_cmdFd = -1;

static void onSignal(int)
{
    g_stop = 1;
    if (g_cmdFd >= 0) {
        char cmd = OC_CMD_CANCEL;
        int r = ::write(g_cmdFd, &cmd, 1);
        (void)r;
    }
}
#endif

// Runs the connection and blocks until it drops. Returns 0 on clean connect.
static int runConnection(const Options& opt, int readyFd)
{
    StoredServer* ss = new StoredServer();
    QString profileArg = opt.profile;
    if (ss->load(profileArg) < 0) {
        err() << "error: profile '" << opt.profile << "' not found\n";
        err().flush();
        delete ss;
        return 2;
    }
    // command-line values override the stored profile
    if (!opt.username.isEmpty()) ss->set_username(opt.username);
    if (!opt.group.isEmpty()) ss->set_groupname(opt.group);

    QString password = opt.password;
    if (password.isEmpty()) {
        password = ss->get_password();
    } else {
        ss->set_password(password);
    }

    ConsoleUi ui(isatty(0) != 0, opt.trustTofu);
    ui.setUsername(ss->get_username());
    ui.setPassword(password);
    ui.setGroup(ss->get_groupname());
    ui.setPin(opt.pin);

    VpnInfo* vpn = nullptr;
    try {
        vpn = new VpnInfo(QStringLiteral("OpenConnect CLI VPN Agent"), ss, &ui);
    } catch (std::exception& ex) {
        err() << "error: cannot initialize VPN: " << ex.what() << '\n';
        err().flush();
        return 3;
    }
    if (opt.logLevel >= 0) {
        vpn->set_default_log_level(opt.logLevel);
    }
    vpn->set_profile_name(opt.profile);

    QUrl turl = QUrl::fromUserInput(
        ss->get_server_gateway().contains(QStringLiteral("://"))
            ? ss->get_server_gateway()
            : QStringLiteral("https://") + ss->get_server_gateway());
    vpn->setUrl(turl);

    int cmdFd = vpn->get_cmd_fd();
    if (cmdFd == INVALID_SOCKET) {
        err() << "error: cannot establish IPC with openconnect\n";
        err().flush();
        delete vpn;
        return 3;
    }

#ifndef _WIN32
    g_cmdFd = cmdFd;
    signal(SIGINT, onSignal);
    signal(SIGTERM, onSignal);
#endif

    // connect() performs the TLS/handshake and the CSTP/DTLS setup
    bool pass_was_empty = ss->get_password().isEmpty();
    int retries = 2;
    int ret = 0;
    do {
        ret = vpn->connect();
        if (ret != 0 && vpn->server_needs_client_cert) {
            err() << "error: the server requires a client certificate and this "
                     "profile has none configured (set one via the GUI or the "
                     "profile's certificate fields)\n";
            err().flush();
            delete vpn;
            return 5;
        }
        if (ret != 0 && pass_was_empty && retries-- > 0) {
            ss->clear_password();
            ss->clear_groupname();
            vpn->reset_vpn();
            continue;
        }
        break;
    } while (true);

    if (ret != 0) {
        err() << "error: " << vpn->last_err << '\n';
        err().flush();
        delete vpn;
        return 4;
    }

    QString ip, ip6, dns, cstp, dtls;
    vpn->get_info(dns, ip, ip6);
    vpn->get_cipher_info(cstp, dtls);
    ss->save();

    // Signal readiness to the parent (daemon mode) and write the state file.
    if (readyFd >= 0) {
        QByteArray line = QStringLiteral("ok %1 %2 %3\n").arg(ip, dns, cstp).toUtf8();
        int r = ocg_write(readyFd, line.constData(), line.size());
        (void)r;
        ocg_close(readyFd);
    }

    {
        QFile sf(stateFileFor(opt.profile));
        if (sf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QTextStream ts(&sf);
            ts << "pid=" << ocg_getpid() << '\n'
               << "profile=" << opt.profile << '\n'
               << "ip=" << ip << '\n'
               << "dns=" << dns << '\n'
               << "cstp=" << cstp << '\n'
               << "dtls=" << dtls << '\n';
        }
    }

    out() << "connected ip=" << ip;
    if (!dns.isEmpty()) out() << " dns=" << dns;
    out() << '\n';
    out().flush();

    vpn->mainloop();

    QFile::remove(stateFileFor(opt.profile));
    out() << "disconnected\n";
    out().flush();
    delete vpn;
    return 0;
}

// ---- commands --------------------------------------------------------------

static int cmdList(const Options& opt)
{
    QStringList names = profileNames();
    if (opt.json) {
        out() << "[";
        for (int i = 0; i < names.size(); ++i) {
            out() << (i ? "," : "") << "\"" << names.at(i) << "\"";
        }
        out() << "]\n";
    } else if (names.isEmpty()) {
        out() << "(no profiles)\n";
    } else {
        for (const QString& n : names) out() << n << '\n';
    }
    out().flush();
    return 0;
}

static int cmdStatus(const Options& opt)
{
    QStringList names = opt.profile.isEmpty() ? profileNames() : QStringList{ opt.profile };
    bool anyOutput = false;
    for (const QString& n : names) {
        QFile sf(stateFileFor(n));
        if (!sf.open(QIODevice::ReadOnly)) {
            continue;
        }
        QString content = QString::fromUtf8(sf.readAll());
        anyOutput = true;
        if (opt.json) {
            out() << "{\"profile\":\"" << n << "\",";
            for (const QString& line : content.split('\n', Qt::SkipEmptyParts)) {
                int eq = line.indexOf('=');
                if (eq > 0) {
                    out() << "\"" << line.left(eq) << "\":\"" << line.mid(eq + 1) << "\",";
                }
            }
            out() << "\"connected\":true}\n";
        } else {
            out() << n << ": connected\n";
            for (const QString& line : content.split('\n', Qt::SkipEmptyParts)) {
                out() << "  " << line << '\n';
            }
        }
    }
    if (!anyOutput && !opt.json) {
        out() << (opt.profile.isEmpty() ? "(no active connections)\n"
                                        : opt.profile + ": disconnected\n");
    }
    out().flush();
    return 0;
}

static int cmdDisconnect(const Options& opt)
{
    QStringList names = opt.profile.isEmpty() ? profileNames() : QStringList{ opt.profile };
    int stopped = 0;
    for (const QString& n : names) {
        QFile sf(stateFileFor(n));
        if (!sf.open(QIODevice::ReadOnly)) {
            continue;
        }
        QString content = QString::fromUtf8(sf.readAll());
        sf.close();
        int pid = -1;
        for (const QString& line : content.split('\n', Qt::SkipEmptyParts)) {
            if (line.startsWith(QStringLiteral("pid="))) {
                pid = line.mid(4).toInt();
            }
        }
#ifndef _WIN32
        if (pid > 0 && ::kill(pid, SIGTERM) == 0) {
            stopped++;
            out() << "disconnected " << n << " (pid " << pid << ")\n";
        } else {
            err() << "warning: " << n << ": process not running, cleaning up\n";
            QFile::remove(stateFileFor(n));
        }
#else
        Q_UNUSED(pid);
#endif
    }
    out().flush();
    return stopped > 0 ? 0 : 1;
}

static int cmdConnect(const Options& opt)
{
    if (!QFile::exists(stateFileFor(opt.profile)) == false) {
        err() << "error: profile '" << opt.profile
              << "' is already connected (use 'disconnect' first)\n";
        err().flush();
        return 1;
    }

    if (opt.daemon) {
#ifndef _WIN32
        int pipefd[2];
        if (::pipe(pipefd) != 0) {
            err() << "error: pipe() failed\n";
            return 3;
        }
        pid_t pid = ::fork();
        if (pid < 0) {
            err() << "error: fork() failed\n";
            return 3;
        }
        if (pid == 0) {
            // child: detach and run
            ::close(pipefd[0]);
            ::setsid();
            int rc = runConnection(opt, pipefd[1]);
            _exit(rc);
        }
        // parent: wait for readiness
        ::close(pipefd[1]);
        char buf[512];
        ssize_t n = ::read(pipefd[0], buf, sizeof(buf) - 1);
        ::close(pipefd[0]);
        if (n > 0) {
            buf[n] = '\0';
            out() << QString::fromUtf8(buf).trimmed() << '\n';
            out() << "started in background (pid " << pid << ")\n";
            out().flush();
            return 0;
        }
        err() << "error: connection failed to start\n";
        err().flush();
        return 4;
#else
        err() << "error: --daemon is not supported on this platform\n";
        return 3;
#endif
    }

    return runConnection(opt, -1);
}

// ---- main ------------------------------------------------------------------

int main(int argc, char** argv)
{
    QCoreApplication::setApplicationName(QStringLiteral("ocg-cli"));
    QCoreApplication::setApplicationVersion(PROJECT_VERSION);
    QCoreApplication::setOrganizationName(QStringLiteral("OpenConnect-GUI Team"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gui.openconnect-vpn.net"));

    // Encrypted key / PKCS#12 passphrase prompt for the CLI.
    set_key_password_prompt([](const QString& title, const QString& label, QString& out) {
        QTextStream es(stderr);
        es << title << ": " << label << ": ";
        es.flush();
        out = QString::fromUtf8(QTextStream(stdin).readLine().toUtf8());
        return !out.isEmpty();
    });

    Options opt;
    QStringList args;
    for (int i = 1; i < argc; ++i) {
        args << QString::fromLocal8Bit(argv[i]);
    }

    // first non-option token is the command
    int i = 0;
    for (; i < args.size(); ++i) {
        const QString& a = args.at(i);
        if (!a.startsWith('-')) {
            opt.command = a;
            ++i;
            break;
        }
        if (a == "-h" || a == "--help") { printUsage(); return 0; }
        if (a == "-v" || a == "--version") { out() << "ocg-cli " << PROJECT_VERSION << '\n'; out().flush(); return 0; }
    }

    for (; i < args.size(); ++i) {
        const QString& a = args.at(i);
        auto next = [&](QString& dst) -> bool {
            if (i + 1 < args.size()) { dst = args.at(++i); return true; }
            err() << "error: missing value for " << a << '\n';
            return false;
        };
        if (a == "--json") { opt.json = true; }
        else if (a == "--trust-tofu") { opt.trustTofu = true; }
        else if (a == "--daemon") { opt.daemon = true; }
        else if (a == "--foreground") { opt.daemon = false; }
        else if (a == "--username") { if (!next(opt.username)) return 2; }
        else if (a == "--group") { if (!next(opt.group)) return 2; }
        else if (a == "--pin") { if (!next(opt.pin)) return 2; }
        else if (a == "--password-file") {
            QString f;
            if (!next(f)) return 2;
            if (f == QStringLiteral("-")) {
                QTextStream in(stdin);
                opt.password = in.readLine();
            } else {
                QFile pf(f);
                if (!pf.open(QIODevice::ReadOnly)) {
                    err() << "error: cannot read password file: " << f << '\n';
                    return 2;
                }
                QTextStream in(&pf);
                opt.password = in.readLine();
            }
        }
        else if (a == "--log-level") {
            QString lv;
            if (!next(lv)) return 2;
            if (lv == "err") opt.logLevel = 0;
            else if (lv == "info") opt.logLevel = 1;
            else if (lv == "debug") opt.logLevel = 2;
            else if (lv == "trace") opt.logLevel = 3;
        }
        else if (a == "-h" || a == "--help") { printUsage(); return 0; }
        else if (a == "-v" || a == "--version") { out() << "ocg-cli " << PROJECT_VERSION << '\n'; out().flush(); return 0; }
        else if (opt.profile.isEmpty() && !a.startsWith('-')) { opt.profile = a; }
        else { err() << "warning: ignoring unknown option " << a << '\n'; }
    }

    if (opt.command.isEmpty()) {
        printUsage();
        return 2;
    }

    // `connect` daemonizes by default; --foreground keeps it attached.
    if (opt.command == QStringLiteral("connect")) {
        if (args.contains(QStringLiteral("--foreground"))) {
            opt.daemon = false;
        } else {
            opt.daemon = true;
        }
    }

    if (opt.command == QStringLiteral("list")) return cmdList(opt);
    if (opt.command == QStringLiteral("status")) return cmdStatus(opt);
    if (opt.command == QStringLiteral("disconnect")) return cmdDisconnect(opt);
    if (opt.command == QStringLiteral("connect")) {
        if (opt.profile.isEmpty()) {
            err() << "error: 'connect' needs a profile name\n";
            return 2;
        }
        return cmdConnect(opt);
    }

    err() << "error: unknown command '" << opt.command << "'\n";
    printUsage();
    return 2;
}
