/*
 * ProfileCard: Modern VPN profile card widget matching dashboard design
 */

#include "ProfileCard.h"
#include "mainwindow.h" // For status_t enum

#include <QContextMenuEvent>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QClipboard>
#include <QApplication>

// Custom widget to render the left icon inside a rounded square
class CardIconWidget : public QWidget {
public:
    CardIconWidget(int iconIndex, QWidget* parent = nullptr)
        : QWidget(parent), m_index(iconIndex)
    {
        setFixedSize(44, 44);
    }

protected:
    void paintEvent(QPaintEvent* /*event*/) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // Rounded container background
        QRectF r(0, 0, width(), height());
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#1e2736"));
        p.drawRoundedRect(r, 10, 10);

        QRectF iconRect = r.adjusted(8, 8, -8, -8);
        ProfileIcons::paint(p, m_index, iconRect, QColor("#a0b2c6"));
    }

private:
    int m_index;
};

ProfileCard::ProfileCard(const QString& profileName,
                         const QString& gateway,
                         const QString& protocolName,
                         int iconIndex,
                         QWidget* parent)
    : QFrame(parent)
    , m_profileName(profileName)
    , m_gateway(gateway)
    , m_protocolName(protocolName)
    , m_cipher(QStringLiteral("AES-256-GCM"))
    , m_dns(QStringLiteral("-"))
    , m_dl(QStringLiteral("0B"))
    , m_up(QStringLiteral("0B"))
    , m_status(STATUS_DISCONNECTED)
    , m_iconIndex(iconIndex)
    , m_adEnabled(false)
    , m_adUserDays(-999)
    , m_adSrvDays(-999)
    , m_adWidget(nullptr)
    , m_adUserBadge(nullptr)
    , m_adSrvBadge(nullptr)
{
    setupUi();
    setConnectionStatus(STATUS_DISCONNECTED);
}

void ProfileCard::setupUi()
{
    setObjectName("profileCard");
    setStyleSheet(QStringLiteral(
        "#profileCard {"
        "   background-color: #161c26;"
        "   border: 1px solid #232d3d;"
        "   border-radius: 12px;"
        "}"
        "#profileCard:hover {"
        "   background-color: #18202c;"
        "   border: 1px solid #2a374a;"
        "}"
    ));

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 16, 18, 16);
    mainLayout->setSpacing(12);

    // --- TOP ROW: Icon + Title & Gateway + Toggle Switch ---
    QHBoxLayout* topRow = new QHBoxLayout();
    topRow->setSpacing(14);

    m_iconWidget = new CardIconWidget(m_iconIndex, this);
    topRow->addWidget(m_iconWidget);

    QVBoxLayout* titleBox = new QVBoxLayout();
    titleBox->setSpacing(3);

    m_titleLabel = new QLabel(m_profileName, this);
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff;");

    m_gatewayLabel = new QLabel(m_gateway, this);
    m_gatewayLabel->setStyleSheet("font-size: 13px; color: #8494a8;");

    titleBox->addWidget(m_titleLabel);
    titleBox->addWidget(m_gatewayLabel);
    topRow->addLayout(titleBox);

    topRow->addStretch();

    m_toggleSwitch = new ToggleSwitch(this);
    connect(m_toggleSwitch, &QAbstractButton::clicked, this, &ProfileCard::onToggleClicked);
    topRow->addWidget(m_toggleSwitch);

    mainLayout->addLayout(topRow);

    // --- MIDDLE ROW: Badges (Protocol + Cipher) ---
    QHBoxLayout* badgesRow = new QHBoxLayout();
    badgesRow->setSpacing(8);

    m_protocolBadge = new QLabel(this);
    m_cipherBadge = new QLabel(this);

    badgesRow->addWidget(m_protocolBadge);
    badgesRow->addWidget(m_cipherBadge);
    badgesRow->addStretch();

    mainLayout->addLayout(badgesRow);

    // --- AD PASSWORD EXPIRY ROW ---
    m_adWidget = new QWidget(this);
    QHBoxLayout* adLayout = new QHBoxLayout(m_adWidget);
    adLayout->setContentsMargins(0, 0, 0, 0);
    adLayout->setSpacing(8);

    m_adUserBadge = new QLabel(m_adWidget);
    m_adSrvBadge = new QLabel(m_adWidget);

    adLayout->addWidget(m_adUserBadge);
    adLayout->addWidget(m_adSrvBadge);
    adLayout->addStretch();

    m_adWidget->setLayout(adLayout);
    m_adWidget->setVisible(false);
    mainLayout->addWidget(m_adWidget);

// Small server/DNS icon matching mockup
class DnsIconWidget : public QWidget {
public:
    explicit DnsIconWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
        setFixedSize(18, 14);
    }
protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor("#8494a8"), 1.2));
        p.setBrush(Qt::NoBrush);

        p.drawRoundedRect(QRectF(1, 1, 16, 5), 1, 1);
        p.drawRoundedRect(QRectF(1, 8, 16, 5), 1, 1);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#8494a8"));
        p.drawEllipse(QPointF(4, 3.5), 1, 1);
        p.drawEllipse(QPointF(4, 10.5), 1, 1);
    }
};

    // --- BOTTOM ROW: DNS + DL/UP stats + Loglar button ---
    QHBoxLayout* bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(8);

    DnsIconWidget* dnsIcon = new DnsIconWidget(this);
    bottomRow->addWidget(dnsIcon);

    m_dnsLabel = new QLabel(this);
    m_dnsLabel->setStyleSheet("font-size: 12px; color: #8494a8; font-family: monospace, menlo, sans-serif;");
    bottomRow->addWidget(m_dnsLabel);

    bottomRow->addStretch();

    m_statsLabel = new QLabel(this);
    m_statsLabel->setStyleSheet("font-size: 12px; color: #8494a8; font-family: monospace, menlo, sans-serif;");
    bottomRow->addWidget(m_statsLabel);

    bottomRow->addSpacing(8);

    m_logButton = new QPushButton(tr(">_ Loglar"), this);
    m_logButton->setCursor(Qt::PointingHandCursor);
    m_logButton->setFixedHeight(28);
    m_logButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #1a222f;"
        "   border: 1px solid #2b3648;"
        "   border-radius: 6px;"
        "   color: #c3d1e4;"
        "   font-size: 12px;"
        "   font-weight: 500;"
        "   padding: 4px 14px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #242f40;"
        "   border: 1px solid #3b4b63;"
        "   color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #151b26;"
        "}"
    );
    connect(m_logButton, &QPushButton::clicked, this, &ProfileCard::logsRequested);
    bottomRow->addWidget(m_logButton);

    mainLayout->addLayout(bottomRow);

    updateBadgeStyles();
}

void ProfileCard::updateBadgeStyles()
{
    QString protoDotColor = "#475569";
    QString protoTextColor = "#94a3b8";

    if (m_status == STATUS_CONNECTED) {
        protoDotColor = "#00d2d3";
        protoTextColor = "#00d2d3";
    } else if (m_status == STATUS_CONNECTING || m_status == STATUS_DISCONNECTING) {
        protoDotColor = "#f59e0b";
        protoTextColor = "#fcd34d";
    }

    QString displayProto = m_protocolName.isEmpty() ? QStringLiteral("Cisco AnyConnect") : m_protocolName;
    m_protocolBadge->setText(QString("<span style='color:%1; font-size:14px;'>●</span> <span style='color:%2; font-weight:500;'>%3</span>")
        .arg(protoDotColor, protoTextColor, displayProto));
    m_protocolBadge->setStyleSheet(
        "background-color: #1b2330;"
        "border: 1px solid #263346;"
        "border-radius: 11px;"
        "padding: 3px 10px;"
        "font-size: 11px;"
    );

    QString displayCipher = m_cipher.isEmpty() ? QStringLiteral("AES-256-GCM") : m_cipher;
    m_cipherBadge->setText(QString("<span style='color:#94a3b8; font-weight:500;'>%1</span>").arg(displayCipher));
    m_cipherBadge->setStyleSheet(
        "background-color: #1b2330;"
        "border: 1px solid #263346;"
        "border-radius: 11px;"
        "padding: 3px 10px;"
        "font-size: 11px;"
    );

    // DNS & Stats
    m_dnsLabel->setText(QString("DNS: %1").arg(m_dns.isEmpty() ? QStringLiteral("-") : m_dns));
    m_statsLabel->setText(QString("DL: %1  UP: %2").arg(m_dl, m_up));

    // AD Password Expiry Badges
    if (!m_adEnabled || !m_adWidget) {
        if (m_adWidget) {
            m_adWidget->setVisible(false);
        }
    } else {
        m_adWidget->setVisible(true);

        QString userDotColor = "#10b981"; // green
        if (m_adUserDays == -999) {
            userDotColor = "#64748b"; // gray
        } else if (m_adUserDays < 0) {
            userDotColor = "#ef4444"; // red
        } else if (m_adUserDays <= 5) {
            userDotColor = "#ef4444"; // red
        } else if (m_adUserDays <= 15) {
            userDotColor = "#f59e0b"; // amber
        }

        QString uText = m_adUserText.isEmpty() ? (m_status == STATUS_CONNECTED ? tr("Sorgulanıyor...") : tr("Bekleniyor...")) : m_adUserText;
        m_adUserBadge->setText(QString("<span style='color:%1; font-size:13px;'>●</span> <span style='color:#94a3b8; font-weight:500;'>Şifre:</span> <span style='color:#e2e8f0; font-weight:600;'>%2</span>")
            .arg(userDotColor, uText));
        m_adUserBadge->setStyleSheet(
            "background-color: #1b2330;"
            "border: 1px solid #263346;"
            "border-radius: 11px;"
            "padding: 3px 10px;"
            "font-size: 11px;"
        );

        if (!m_adSrvUser.isEmpty()) {
            m_adSrvBadge->setVisible(true);
            QString srvDotColor = "#10b981";
            if (m_adSrvDays == -999) {
                srvDotColor = "#64748b";
            } else if (m_adSrvDays < 0) {
                srvDotColor = "#ef4444";
            } else if (m_adSrvDays <= 5) {
                srvDotColor = "#ef4444";
            } else if (m_adSrvDays <= 15) {
                srvDotColor = "#f59e0b";
            }

            QString sText = m_adSrvText.isEmpty() ? (m_status == STATUS_CONNECTED ? tr("Sorgulanıyor...") : tr("Bekleniyor...")) : m_adSrvText;
            m_adSrvBadge->setText(QString("<span style='color:%1; font-size:13px;'>●</span> <span style='color:#94a3b8; font-weight:500;'>SRV:</span> <span style='color:#e2e8f0; font-weight:600;'>%2</span>")
                .arg(srvDotColor, sText));
            m_adSrvBadge->setStyleSheet(
                "background-color: #1b2330;"
                "border: 1px solid #263346;"
                "border-radius: 11px;"
                "padding: 3px 10px;"
                "font-size: 11px;"
            );
            m_adSrvBadge->setToolTip(tr("SRV Hesabı: %1").arg(m_adSrvUser));
        } else {
            m_adSrvBadge->setVisible(false);
        }
    }
}

void ProfileCard::setAdSettings(bool enabled,
                                const QString& domain,
                                const QString& srvUser,
                                const QString& userText,
                                int userDays,
                                const QString& srvText,
                                int srvDays)
{
    m_adEnabled = enabled;
    m_adDomain = domain;
    m_adSrvUser = srvUser;
    m_adUserText = userText;
    m_adUserDays = userDays;
    m_adSrvText = srvText;
    m_adSrvDays = srvDays;
    updateBadgeStyles();
}

void ProfileCard::setAdExpiryInfo(const QString& userText, int userDays, const QString& srvText, int srvDays)
{
    m_adUserText = userText;
    m_adUserDays = userDays;
    m_adSrvText = srvText;
    m_adSrvDays = srvDays;
    updateBadgeStyles();
}

void ProfileCard::setConnectionStatus(int status)
{
    m_status = status;

    if (status == STATUS_CONNECTED) {
        m_toggleSwitch->setCheckedSilent(true);
        m_toggleSwitch->setConnecting(false);
    } else if (status == STATUS_CONNECTING || status == STATUS_DISCONNECTING) {
        m_toggleSwitch->setCheckedSilent(status == STATUS_CONNECTING);
        m_toggleSwitch->setConnecting(true);
    } else {
        m_toggleSwitch->setCheckedSilent(false);
        m_toggleSwitch->setConnecting(false);
        m_dl = QStringLiteral("0B");
        m_up = QStringLiteral("0B");
    }

    updateBadgeStyles();
}

void ProfileCard::setDns(const QString& dns)
{
    m_dns = dns;
    updateBadgeStyles();
}

void ProfileCard::setStats(const QString& dl, const QString& up)
{
    m_dl = dl.isEmpty() ? QStringLiteral("0B") : dl;
    m_up = up.isEmpty() ? QStringLiteral("0B") : up;
    updateBadgeStyles();
}

void ProfileCard::setCipher(const QString& cipher)
{
    if (!cipher.isEmpty()) {
        m_cipher = cipher;
        updateBadgeStyles();
    }
}

void ProfileCard::onToggleClicked(bool checked)
{
    emit toggleRequested(checked);
}

void ProfileCard::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu;
    menu.setStyleSheet(
        "QMenu {"
        "   background-color: #1a222f;"
        "   border: 1px solid #2c3a4f;"
        "   color: #e2e8f0;"
        "   padding: 5px;"
        "}"
        "QMenu::item {"
        "   padding: 6px 20px;"
        "   border-radius: 4px;"
        "}"
        "QMenu::item:selected {"
        "   background-color: #2b384c;"
        "   color: #ffffff;"
        "}"
    );

    QAction* actConnect = nullptr;
    if (m_status == STATUS_CONNECTED) {
        actConnect = menu.addAction(tr("Bağlantıyı Kes (Disconnect)"));
    } else if (m_status == STATUS_DISCONNECTED) {
        actConnect = menu.addAction(tr("Bağlan (Connect)"));
    }

    QAction* actRefreshAd = nullptr;
    if (m_adEnabled && m_status == STATUS_CONNECTED) {
        actRefreshAd = menu.addAction(tr("AD Şifre Süresini Yenile"));
    }

    menu.addSeparator();

    QAction* actCopyGw = menu.addAction(tr("Ağ Geçidini Kopyala"));
    QAction* actEdit = menu.addAction(tr("Profili Düzenle"));
    QAction* actDelete = menu.addAction(tr("Profili Sil"));

    QAction* selected = menu.exec(event->globalPos());
    if (!selected) {
        return;
    }

    if (selected == actConnect) {
        emit toggleRequested(m_status == STATUS_DISCONNECTED);
    } else if (selected == actRefreshAd) {
        emit adRefreshRequested();
    } else if (selected == actCopyGw) {
        QApplication::clipboard()->setText(m_gateway);
    } else if (selected == actEdit) {
        emit editRequested();
    } else if (selected == actDelete) {
        emit deleteRequested();
    }
}

void ProfileCard::paintEvent(QPaintEvent* event)
{
    QFrame::paintEvent(event);
}
