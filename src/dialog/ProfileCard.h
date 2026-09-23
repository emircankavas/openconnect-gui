/*
 * ProfileCard: Modern VPN profile card widget matching dashboard design
 */

#pragma once

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "ToggleSwitch.h"
#include "ProfileIcons.h"

class ProfileCard : public QFrame {
    Q_OBJECT

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

    int status() const { return m_status; }

signals:
    void toggleRequested(bool connect);
    void logsRequested();
    void editRequested();
    void deleteRequested();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onToggleClicked(bool checked);

private:
    QString m_profileName;
    QString m_gateway;
    QString m_protocolName;
    QString m_cipher;
    QString m_dns;
    QString m_dl;
    QString m_up;
    int m_status;
    int m_iconIndex;

    // UI Widgets
    QWidget* m_iconWidget;
    QLabel* m_titleLabel;
    QLabel* m_gatewayLabel;
    ToggleSwitch* m_toggleSwitch;

    QLabel* m_protocolBadge;
    QLabel* m_cipherBadge;

    QLabel* m_dnsLabel;
    QLabel* m_statsLabel;
    QPushButton* m_logButton;

    void setupUi();
    void updateBadgeStyles();
};

