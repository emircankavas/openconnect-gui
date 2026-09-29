/*
 * ProfileCard: Modern VPN profile card widget matching dashboard design
 */

#include "ProfileCard.h"
#include "mainwindow.h" // For status_t enum

#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QEasingCurve>
#include <QMenu>
#include <QPainter>
#include <QToolTip>

// Lightweight icon widget without heavy boxed background
class CardIconWidget : public QWidget {
public:
    CardIconWidget(int iconIndex, QWidget* parent = nullptr)
        : QWidget(parent), m_index(iconIndex), m_connected(false)
    {
        setFixedSize(28, 28);
    }

    void setConnected(bool connected)
    {
        if (m_connected != connected) {
            m_connected = connected;
            update();
        }
    }

protected:
    void paintEvent(QPaintEvent* /*event*/) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QRectF iconRect = rect().adjusted(1, 1, -1, -1);
        ProfileIcons::paint(p, m_index, iconRect, m_connected ? QColor("#00d2d3") : QColor("#8494a8"));
    }

private:
    int m_index;
    bool m_connected;
};

// Custom modern 3-dot menu button
class CardMenuButton : public QPushButton {
public:
    explicit CardMenuButton(QWidget* parent = nullptr) : QPushButton(parent)
    {
        setFixedSize(28, 28);
        setCursor(Qt::PointingHandCursor);
        setToolTip(tr("Seçenekler"));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        if (isDown()) {
            p.setBrush(QColor("#151d27"));
            p.setPen(QColor("#2c3b4e"));
            p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 6, 6);
        } else if (underMouse()) {
            p.setBrush(QColor("#202b3a"));
            p.setPen(QColor("#2c3b4e"));
            p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 6, 6);
        }

        // Draw 3 vertical dots
        p.setPen(Qt::NoPen);
        p.setBrush(underMouse() ? QColor("#ffffff") : QColor("#8494a8"));
        qreal cx = width() / 2.0;
        qreal cy = height() / 2.0;
        qreal r = 1.6;
        p.drawEllipse(QPointF(cx, cy - 5.5), r, r);
        p.drawEllipse(QPointF(cx, cy), r, r);
        p.drawEllipse(QPointF(cx, cy + 5.5), r, r);
    }
};

// Sleek padlock widget for cipher info with hover tooltip and click-to-copy
class CipherLockWidget : public QWidget {
public:
    explicit CipherLockWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
        setFixedSize(20, 20);
        setCursor(Qt::PointingHandCursor);
    }

    void setCipher(const QString& cipher)
    {
        m_cipher = cipher;
        setToolTip(tr("Şifreleme (Cipher):\n%1\n(Kopyalamak için tıklayın)").arg(m_cipher));
    }

protected:
    void enterEvent(QEnterEvent*) override { update(); }
    void leaveEvent(QEvent*) override { update(); }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && !m_cipher.isEmpty()) {
            QApplication::clipboard()->setText(m_cipher);
            QToolTip::showText(mapToGlobal(QPoint(0, height())), tr("Şifreleme bilgisi kopyalandı!"), this);
        }
        QWidget::mousePressEvent(event);
    }

    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        if (underMouse()) {
            p.setPen(QPen(QColor("#2c3c50"), 1));
            p.setBrush(QColor("#1b2533"));
            p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 4, 4);
        }

        // Draw padlock
        QColor lockColor = underMouse() ? QColor("#38bdf8") : QColor("#8494a8");
        p.setPen(QPen(lockColor, 1.3));
        p.setBrush(Qt::NoBrush);

        // Shackle
        QRectF shackle(6, 3.5, 8, 7.5);
        p.drawArc(shackle, 0, 180 * 16);
        p.drawLine(QPointF(6, 7.5), QPointF(6, 9.5));
        p.drawLine(QPointF(14, 7.5), QPointF(14, 9.5));

        // Lock Body
        p.setBrush(lockColor);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(4.5, 9, 11, 7.5), 1.5, 1.5);

        // Keyhole
        p.setBrush(QColor("#0d121a"));
        p.drawEllipse(QPointF(10, 11.8), 1.0, 1.0);
        p.drawRect(QRectF(9.6, 11.8, 0.8, 2.0));
    }

private:
    QString m_cipher;
};

// Subtle inner container / chip for active connection metrics
class MetricsChip : public QFrame {
public:
    explicit MetricsChip(QWidget* parent = nullptr) : QFrame(parent)
    {
        setObjectName("metricsChip");
        setStyleSheet(QStringLiteral(
            "#metricsChip {"
            "   background-color: #0c1118;"
            "   border: 1px solid #1c2635;"
            "   border-radius: 7px;"
            "}"
        ));
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        if (height() < 8) return;
        QFrame::paintEvent(event);
    }
};

static QString formatProtocolTag(const QString& proto)
{
    if (proto.contains(QStringLiteral("AnyConnect"), Qt::CaseInsensitive)) return QStringLiteral("AnyConnect");
    if (proto.contains(QStringLiteral("Fortinet"), Qt::CaseInsensitive)) return QStringLiteral("Fortinet");
    if (proto.contains(QStringLiteral("GlobalProtect"), Qt::CaseInsensitive)) return QStringLiteral("GlobalProtect");
    if (proto.contains(QStringLiteral("Pulse"), Qt::CaseInsensitive)) return QStringLiteral("Pulse");
    if (proto.contains(QStringLiteral("F5"), Qt::CaseInsensitive)) return QStringLiteral("F5");
    return proto.isEmpty() ? QStringLiteral("AnyConnect") : proto;
}

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
    , m_metricsHeight(0)
    , m_adEnabled(false)
    , m_adUserDays(-999)
    , m_adSrvDays(-999)
    , m_iconWidget(nullptr)
    , m_titleLabel(nullptr)
    , m_adLabel(nullptr)
    , m_protocolBadge(nullptr)
    , m_toggleSwitch(nullptr)
    , m_menuButton(nullptr)
    , m_metricsChip(nullptr)
    , m_dnsLabel(nullptr)
    , m_statsLabel(nullptr)
    , m_cipherLockWidget(nullptr)
    , m_metricsAnim(nullptr)
{
    setupUi();
    setConnectionStatus(STATUS_DISCONNECTED);
}

void ProfileCard::setupUi()
{
    setObjectName("profileCard");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 9, 14, 9);
    mainLayout->setSpacing(6);

    // --- TOP ROW: [Icon] [Title + AD Expiry] ----- [Protocol Pill] [Toggle Switch] [⋮ Menu] ---
    QHBoxLayout* topRow = new QHBoxLayout();
    topRow->setContentsMargins(0, 0, 0, 0);
    topRow->setSpacing(10);

    // 1. Left: Pure crisp icon without heavy boxed background
    m_iconWidget = new CardIconWidget(m_iconIndex, this);
    topRow->addWidget(m_iconWidget, 0, Qt::AlignVCenter);

    // 2. Center-Left: Profile Name + AD Expiry (with calibrated vertical spacing)
    QVBoxLayout* titleBox = new QVBoxLayout();
    titleBox->setContentsMargins(0, 0, 0, 0);
    titleBox->setSpacing(4);

    m_titleLabel = new QLabel(m_profileName, this);
    m_titleLabel->setStyleSheet("font-size: 15px; font-weight: 600; color: #f8fafc;");
    m_titleLabel->setToolTip(m_profileName);
    titleBox->addWidget(m_titleLabel);

    m_adLabel = new QLabel(this);
    m_adLabel->setStyleSheet("font-size: 11px; color: #8494a8;");
    m_adLabel->setVisible(false);
    titleBox->addWidget(m_adLabel);

    topRow->addLayout(titleBox, 1);

    // 3. Right: Fixed-alignment actions block (Protocol Badge | Switch | Menu)
    // Locked in exact columns across all cards to eliminate zigzagging!
    QHBoxLayout* rightBlock = new QHBoxLayout();
    rightBlock->setContentsMargins(0, 0, 0, 0);
    rightBlock->setSpacing(8);

    m_protocolBadge = new QLabel(this);
    m_protocolBadge->setFixedWidth(92);
    m_protocolBadge->setAlignment(Qt::AlignCenter);
    m_protocolBadge->setToolTip(m_protocolName);
    rightBlock->addWidget(m_protocolBadge, 0, Qt::AlignVCenter);

    m_toggleSwitch = new ToggleSwitch(this);
    connect(m_toggleSwitch, &QAbstractButton::clicked, this, &ProfileCard::onToggleClicked);
    rightBlock->addWidget(m_toggleSwitch, 0, Qt::AlignVCenter);

    m_menuButton = new CardMenuButton(this);
    connect(m_menuButton, &QPushButton::clicked, this, [this]() {
        showCardMenu(m_menuButton->mapToGlobal(QPoint(0, m_menuButton->height() + 4)));
    });
    rightBlock->addWidget(m_menuButton, 0, Qt::AlignVCenter);

    topRow->addLayout(rightBlock, 0);
    mainLayout->addLayout(topRow);

    // --- CONTEXT-AWARE EXPANDABLE METRICS CHIP (Inner container panel) ---
    m_metricsChip = new MetricsChip(this);
    m_metricsChip->setFixedHeight(0);
    m_metricsChip->setVisible(false);

    QHBoxLayout* chipLayout = new QHBoxLayout(m_metricsChip);
    chipLayout->setContentsMargins(10, 4, 10, 4);
    chipLayout->setSpacing(12);

    m_dnsLabel = new QLabel(m_metricsChip);
    m_dnsLabel->setStyleSheet("font-size: 11px; color: #8494a8; font-family: -apple-system, Menlo, monospace;");
    chipLayout->addWidget(m_dnsLabel);

    chipLayout->addStretch();

    m_statsLabel = new QLabel(m_metricsChip);
    m_statsLabel->setStyleSheet("font-size: 11px; color: #8494a8; font-family: -apple-system, Menlo, monospace;");
    chipLayout->addWidget(m_statsLabel);

    m_cipherLockWidget = new CipherLockWidget(m_metricsChip);
    m_cipherLockWidget->setCipher(m_cipher);
    chipLayout->addWidget(m_cipherLockWidget);

    mainLayout->addWidget(m_metricsChip);

    m_metricsAnim = new QPropertyAnimation(this, "metricsHeight", this);
    m_metricsAnim->setDuration(220);

    updateBadgeStyles();
}

void ProfileCard::setMetricsHeight(int h)
{
    m_metricsHeight = h;
    if (m_metricsChip) {
        m_metricsChip->setFixedHeight(h);
        m_metricsChip->setVisible(h > 0);
    }
}

QString ProfileCard::formatDays(const QString& text, int days) const
{
    if (days == -999) {
        return text.isEmpty() ? tr("Bekleniyor...") : text;
    }
    if (days < 0) {
        return tr("Doldu!");
    }
    return QString("%1g").arg(days);
}

void ProfileCard::updateBadgeStyles()
{
    // Update card border & background depending on connection status
    if (m_status == STATUS_CONNECTED) {
        setStyleSheet(QStringLiteral(
            "#profileCard {"
            "   background-color: #141c26;"
            "   border: 1px solid #1d3946;"
            "   border-radius: 12px;"
            "}"
            "#profileCard:hover {"
            "   background-color: #17212e;"
            "   border: 1px solid #264e5e;"
            "}"
        ));
    } else {
        setStyleSheet(QStringLiteral(
            "#profileCard {"
            "   background-color: #151b24;"
            "   border: 1px solid #202a38;"
            "   border-radius: 12px;"
            "}"
            "#profileCard:hover {"
            "   background-color: #18202b;"
            "   border: 1px solid #283546;"
            "}"
        ));
    }

    // Protocol badge styling
    QString protoDotColor = "#64748b";
    QString protoTextColor = "#94a3b8";

    if (m_status == STATUS_CONNECTED) {
        protoDotColor = "#10b981";
        protoTextColor = "#34d399";
    } else if (m_status == STATUS_CONNECTING || m_status == STATUS_DISCONNECTING) {
        protoDotColor = "#f59e0b";
        protoTextColor = "#fcd34d";
    }

    QString displayProto = formatProtocolTag(m_protocolName);
    m_protocolBadge->setText(QString("<span style='color:%1; font-size:10px;'>●</span> <span style='color:%2; font-weight:500;'>%3</span>")
        .arg(protoDotColor, protoTextColor, displayProto));
    m_protocolBadge->setStyleSheet(
        "background-color: #1a222e;"
        "border: 1px solid #263345;"
        "border-radius: 8px;"
        "padding: 2px 4px;"
        "font-size: 11px;"
    );

    // DNS & Traffic Metrics inside the inner chip
    m_dnsLabel->setText(QString("<span style='color:#64748b;'>DNS</span> <span style='color:#cbd5e1;'>%1</span>")
        .arg(m_dns.isEmpty() ? QStringLiteral("-") : m_dns));

    m_statsLabel->setText(QString("<span style='color:#38bdf8; font-weight:bold;'>↓</span> <span style='color:#cbd5e1;'>%1</span>   <span style='color:#34d399; font-weight:bold;'>↑</span> <span style='color:#cbd5e1;'>%2</span>")
        .arg(m_dl, m_up));

    if (m_cipherLockWidget) {
        m_cipherLockWidget->setCipher(m_cipher);
    }

    // AD Password Expiry (Clean secondary line below title)
    if (!m_adEnabled) {
        m_adLabel->setVisible(false);
    } else {
        m_adLabel->setVisible(true);

        QString uFormatted = formatDays(m_adUserText, m_adUserDays);
        QString uColor = "#94a3b8";
        if (m_adUserDays >= 0 && m_adUserDays <= 5) {
            uColor = "#ef4444";
        } else if (m_adUserDays >= 0 && m_adUserDays <= 15) {
            uColor = "#f59e0b";
        }

        QString lineText;
        if (!m_adSrvUser.isEmpty()) {
            QString sFormatted = formatDays(m_adSrvText, m_adSrvDays);
            QString sColor = "#94a3b8";
            if (m_adSrvDays >= 0 && m_adSrvDays <= 5) {
                sColor = "#ef4444";
            } else if (m_adSrvDays >= 0 && m_adSrvDays <= 15) {
                sColor = "#f59e0b";
            }

            lineText = QString(
                "<span style='color:#64748b; font-weight:500;'>%1</span> "
                "<span style='color:%2; font-weight:600;'>VPN %3</span> "
                "<span style='color:#475569;'>•</span> "
                "<span style='color:%4; font-weight:600;'>SRV %5</span>"
            ).arg(tr("Kalan:"), uColor, uFormatted, sColor, sFormatted);
        } else {
            lineText = QString(
                "<span style='color:#64748b; font-weight:500;'>%1</span> "
                "<span style='color:%2; font-weight:600;'>VPN %3</span>"
            ).arg(tr("Kalan:"), uColor, uFormatted);
        }

        m_adLabel->setText(lineText);
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

    if (m_iconWidget) {
        m_iconWidget->setConnected(status == STATUS_CONNECTED);
    }

    if (status == STATUS_CONNECTED) {
        m_toggleSwitch->setCheckedSilent(true);
        m_toggleSwitch->setConnecting(false);

        // Expand metrics chip with smooth animation
        if (!isVisible()) {
            setMetricsHeight(30);
        } else {
            m_metricsAnim->stop();
            m_metricsAnim->setStartValue(m_metricsHeight);
            m_metricsAnim->setEndValue(30);
            m_metricsAnim->setEasingCurve(QEasingCurve::OutCubic);
            m_metricsAnim->start();
        }
    } else if (status == STATUS_CONNECTING || status == STATUS_DISCONNECTING) {
        m_toggleSwitch->setCheckedSilent(status == STATUS_CONNECTING);
        m_toggleSwitch->setConnecting(true);

        // Collapse metrics chip
        if (m_metricsHeight > 0) {
            m_metricsAnim->stop();
            m_metricsAnim->setStartValue(m_metricsHeight);
            m_metricsAnim->setEndValue(0);
            m_metricsAnim->setEasingCurve(QEasingCurve::InCubic);
            m_metricsAnim->start();
        }
    } else {
        m_toggleSwitch->setCheckedSilent(false);
        m_toggleSwitch->setConnecting(false);
        m_dl = QStringLiteral("0B");
        m_up = QStringLiteral("0B");

        // Collapse metrics chip
        if (m_metricsHeight > 0) {
            m_metricsAnim->stop();
            m_metricsAnim->setStartValue(m_metricsHeight);
            m_metricsAnim->setEndValue(0);
            m_metricsAnim->setEasingCurve(QEasingCurve::InCubic);
            m_metricsAnim->start();
        }
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

void ProfileCard::showCardMenu(const QPoint& globalPos)
{
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu {"
        "   background-color: #1a222f;"
        "   border: 1px solid #2d3b4e;"
        "   border-radius: 8px;"
        "   color: #e2e8f0;"
        "   padding: 5px;"
        "}"
        "QMenu::item {"
        "   padding: 6px 22px 6px 14px;"
        "   border-radius: 5px;"
        "   font-size: 13px;"
        "}"
        "QMenu::item:selected {"
        "   background-color: #2b3a4f;"
        "   color: #ffffff;"
        "}"
        "QMenu::separator {"
        "   height: 1px;"
        "   background: #283548;"
        "   margin: 4px 6px;"
        "}"
    );

    QAction* actConnect = nullptr;
    if (m_status == STATUS_CONNECTED) {
        actConnect = menu.addAction(tr("Bağlantıyı Kes"));
    } else if (m_status == STATUS_DISCONNECTED) {
        actConnect = menu.addAction(tr("Bağlan"));
    }

    menu.addSeparator();

    QAction* actEdit = menu.addAction(tr("Profili Düzenle"));
    QAction* actLogs = menu.addAction(tr("Logları Görüntüle"));
    QAction* actCopyGw = menu.addAction(tr("Ağ Geçidini Kopyala"));

    QAction* actRefreshAd = nullptr;
    if (m_adEnabled && m_status == STATUS_CONNECTED) {
        actRefreshAd = menu.addAction(tr("AD Şifre Süresini Yenile"));
    }

    menu.addSeparator();
    QAction* actDelete = menu.addAction(tr("Profili Sil"));

    QAction* selected = menu.exec(globalPos);
    if (!selected) {
        return;
    }

    if (selected == actConnect) {
        emit toggleRequested(m_status == STATUS_DISCONNECTED);
    } else if (selected == actEdit) {
        emit editRequested();
    } else if (selected == actLogs) {
        emit logsRequested();
    } else if (selected == actCopyGw) {
        QApplication::clipboard()->setText(m_gateway);
    } else if (selected == actRefreshAd) {
        emit adRefreshRequested();
    } else if (selected == actDelete) {
        emit deleteRequested();
    }
}

void ProfileCard::contextMenuEvent(QContextMenuEvent* event)
{
    showCardMenu(event->globalPos());
}

void ProfileCard::paintEvent(QPaintEvent* event)
{
    QFrame::paintEvent(event);
}
