/*
 * ProfileCard: Modern VPN profile card widget matching dashboard design
 */

#pragma once

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPropertyAnimation>

#include "ToggleSwitch.h"
#include "ProfileIcons.h"

class CardIconWidget;
class CardMenuButton;
class CipherLockWidget;
class MetricsChip;

class ProfileCard : public QFrame {
    Q_OBJECT
    Q_PROPERTY(int metricsHeight READ metricsHeight WRITE setMetricsHeight)

public:
    explicit ProfileCard(const QString& profileName,
                        const QString& gateway,
                        const QString& protocolName,
                        int iconIndex = 0,
                        QWidget* parent = nullptr);

    QString profileName() const { return m_profileName; }
    QString gateway() const { return m_gateway; }
    QString protocolName() const { return m_protocolName; }

    void setConnectionStatus(int status);
    void setDns(const QString& dns);
    void setStats(const QString& dl, const QString& up);
    void setCipher(const QString& cipher);
    void setAdSettings(bool enabled,
                       const QString& domain,
                       const QString& srvUser,
                       const QString& userText = {},
                       int userDays = -999,
                       const QString& srvText = {},
                       int srvDays = -999);
    void setAdExpiryInfo(const QString& userText, int userDays, const QString& srvText, int srvDays);

    int status() const { return m_status; }

    int metricsHeight() const { return m_metricsHeight; }
    void setMetricsHeight(int h);

signals:
    void toggleRequested(bool connect);
    void logsRequested();
    void editRequested();
    void deleteRequested();
    void adRefreshRequested();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onToggleClicked(bool checked);

private:
    void setupUi();
    void updateBadgeStyles();
    void showCardMenu(const QPoint& globalPos);
    QString formatDays(const QString& text, int days) const;

    QString m_profileName;
    QString m_gateway;
    QString m_protocolName;
    QString m_cipher;
    QString m_dns;
    QString m_dl;
    QString m_up;
    int m_status;
    int m_iconIndex;
    int m_metricsHeight;

    bool m_adEnabled;
    QString m_adDomain;
    QString m_adSrvUser;
    QString m_adUserText;
    int m_adUserDays;
    QString m_adSrvText;
    int m_adSrvDays;

    // UI Widgets
    CardIconWidget* m_iconWidget;
    QLabel* m_titleLabel;
    QLabel* m_adLabel;
    QLabel* m_protocolBadge;
    ToggleSwitch* m_toggleSwitch;
    CardMenuButton* m_menuButton;

    // Expandable metrics container (chip style)
    MetricsChip* m_metricsChip;
    QLabel* m_dnsLabel;
    QLabel* m_statsLabel;
    CipherLockWidget* m_cipherLockWidget;
    QPropertyAnimation* m_metricsAnim;
};

