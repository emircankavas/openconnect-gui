/*
 * Copyright (C) 2014 Red Hat
 *
 * This file is part of openconnect-gui.
 *
 * openconnect-gui is free software: you can redistribute it and/or modify
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

#include "common.h"
#include "OcSettings.h"

#include <QCoreApplication>
#include <QFutureWatcher>
#include <QMainWindow>
#include <QMenu>
#include <QMutex>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QNetworkReply>
#include <QProgressDialog>

#ifndef _WIN32
#include <cerrno>
#include <sys/socket.h>
#include <sys/types.h>
#else
#include <winsock2.h>
#endif

extern "C" {
#include <openconnect.h>
}

class LogDialog;
class QStateMachine;
class ProfileCard;
class QVBoxLayout;
class QScrollArea;
class QLabel;

namespace Ui {
class MainWindow;
}
enum status_t {
    STATUS_DISCONNECTING,
    STATUS_DISCONNECTED,
    STATUS_CONNECTING,
    STATUS_CONNECTED
};

#include <memory>
#include <QMap>

struct VpnConnection {
    QString profileName;
    class VpnInfo* vpninfo = nullptr;
    SOCKET cmd_fd = INVALID_SOCKET;
    int status = STATUS_DISCONNECTED;
    QString dns;
    QString ip;
    QString ip6;
    QString cstp_cipher;
    QString dtls_cipher;
    QString tx_bytes;
    QString rx_bytes;
    bool minimize_on_connect = false;
    QFuture<void> future;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = 0, bool useTray = false, const QString profileName = {});
    ~MainWindow();

    void updateStats(const struct oc_stats* stats, QString dtls);
    void updateStats(const QString& profileName, const struct oc_stats* stats, QString dtls);
    void reload_settings();

    void vpn_status_changed(int connected);
    void vpn_status_changed(int connected,
        QString& dns,
        QString& ip,
        QString& ip6,
        QString& cstp_cipher,
        QString& dtls_cipher);
    void vpn_status_changed(const QString& profileName, int connected);
    void vpn_status_changed(const QString& profileName,
        int connected,
        QString& dns,
        QString& ip,
        QString& ip6,
        QString& cstp_cipher,
        QString& dtls_cipher);

    int get_log_level();
    void connectProfile(const QString& profileName);
    void disconnectProfile(const QString& profileName);
    void checkAdPasswordExpiry(const QString& profileName);

public slots:
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void statsChanged(QString, QString, QString);
    void statsChanged(QString profileName, QString tx, QString rx, QString dtls);
    void changeStatus(int);
    void changeStatus(QString profileName, int);

    void blink_ui(void);

    void request_update_stats();

    void on_connectClicked();
    void on_disconnectClicked();
    void on_serverList_currentIndexChanged(int index);

    void closeEvent(QCloseEvent* event) override;

    void on_actionAbout_triggered();
    void on_actionCheckForUpdates_triggered();

    void on_actionNewProfile_triggered();
    void on_actionNewProfileAdvanced_triggered();
    void on_actionEditSelectedProfile_triggered();
    void on_actionRemoveSelectedProfile_triggered();

    void on_actionLicense_triggered();
    void on_actionReport_an_issue_triggered();
    void on_actionWebSite_triggered();

signals:
    void stats_changed_sig(QString, QString, QString);
    void stats_changed_sig(QString profileName, QString tx, QString rx, QString dtls);
    void vpn_status_changed_sig(int);
    void vpn_status_changed_sig(QString profileName, int);
    void timeout(void);
    void readyToShutdown();
    void version_download_completed_sig();

private slots:
    void createLogDialog();
    void tryCheckLatestVersion();
    void checkForUpdatesDialog();

private:
    void gotLatestVersion(QNetworkReply *reply);
    void checkLatestVersion() const;

    static QString normalize_byte_size(uint64_t bytes);
    void createTrayIcon();

    void readSettings();
    void writeSettings();

    std::shared_ptr<VpnConnection> getConnection(const QString& profileName);
    void updateUiForProfile(const QString& profileName);
    void updateServerListItem(const QString& profileName, int status);
    void updateTrayIconState();

    void setupDashboardUi();

    QMap<QString, std::shared_ptr<VpnConnection>> m_connections;
    QMap<QString, ProfileCard*> m_profileCards;
    QVBoxLayout* m_cardsLayout = nullptr;
    QWidget* m_cardsContainer = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QLabel* m_emptyStateLabel = nullptr;

    /* we keep the fd instead of a pointer to vpninfo to avoid
     * any multithread issues */
    SOCKET cmd_fd;
    bool minimize_on_connect;
    Ui::MainWindow* ui;
    QTimer* timer;
    QTimer* blink_timer;
    QFutureWatcher<void> futureWatcher; // watches the vpninfo

    QString dns;
    QString ip;
    QString ip6;
    QString cstp_cipher;
    QString dtls_cipher;

    QString latest_version;
    time_t last_check_time;
    QProgressDialog *downloadProgress;

    QNetworkAccessManager *manager;

    QStateMachine* m_appWindowStateMachine;
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayIconMenu = nullptr;
    QMenu* m_trayIconMenuConnections;
    QAction* m_disconnectAction;
    QAction* m_minimizeAction;
    QAction* m_restoreAction;
    QAction* m_quitAction;
};
