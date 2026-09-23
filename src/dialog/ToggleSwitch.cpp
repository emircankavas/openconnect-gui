/*
 * Modern animated toggle switch widget for Qt
 */

#include "ToggleSwitch.h"

#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <cmath>

ToggleSwitch::ToggleSwitch(QWidget* parent)
    : QAbstractButton(parent)
    , m_offset(0.0)
    , m_trackColor(QColor("#252d3a"))
    , m_connecting(false)
    , m_animOffset(new QPropertyAnimation(this, "offset", this))
    , m_animColor(new QPropertyAnimation(this, "trackColor", this))
    , m_pulseTimer(new QTimer(this))
    , m_pulseStep(0)
{
    setCheckable(true);
    setChecked(false);
    setCursor(Qt::PointingHandCursor);

    m_animOffset->setDuration(180);
    m_animColor->setDuration(180);

    connect(m_pulseTimer, &QTimer::timeout, this, [this]() {
        m_pulseStep = (m_pulseStep + 1) % 360;
        update();
    });
}

QSize ToggleSwitch::sizeHint() const
{
    return QSize(50, 28);
}

void ToggleSwitch::setOffset(qreal o)
{
    m_offset = o;
    update();
}

void ToggleSwitch::setTrackColor(const QColor& c)
{
    m_trackColor = c;
    update();
}

void ToggleSwitch::setConnecting(bool connecting)
{
    if (m_connecting == connecting) return;
    m_connecting = connecting;
    if (m_connecting) {
        m_pulseTimer->start(40);
    } else {
        m_pulseTimer->stop();
    }
    update();
}

void ToggleSwitch::setCheckedSilent(bool checked)
{
    blockSignals(true);
    setChecked(checked);
    m_offset = checked ? 1.0 : 0.0;
    m_trackColor = checked ? QColor("#00c2cb") : QColor("#252d3a");
    blockSignals(false);
    update();
}

void ToggleSwitch::nextCheckState()
{
    QAbstractButton::nextCheckState();

    m_animOffset->stop();
    m_animOffset->setStartValue(m_offset);
    m_animOffset->setEndValue(isChecked() ? 1.0 : 0.0);
    m_animOffset->start();

    m_animColor->stop();
    m_animColor->setStartValue(m_trackColor);
    m_animColor->setEndValue(isChecked() ? QColor("#00c2cb") : QColor("#252d3a"));
    m_animColor->start();
}

void ToggleSwitch::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal w = width();
    const qreal h = height();
    const qreal radius = h / 2.0;

    // Track
    QPainterPath trackPath;
    trackPath.addRoundedRect(QRectF(1, 1, w - 2, h - 2), radius, radius);

    QColor curTrackColor = m_trackColor;
    if (m_connecting) {
        // Pulsing cyan when connecting
        int alpha = 130 + static_cast<int>(100.0 * std::sin(m_pulseStep * 3.14159 / 90.0));
        if (alpha < 60) alpha = 60;
        if (alpha > 255) alpha = 255;
        curTrackColor = QColor(0, 194, 203, alpha);
    }

    p.fillPath(trackPath, curTrackColor);

    // Track border
    QColor borderColor = isChecked() ? QColor("#00c2cb") : QColor("#364253");
    if (m_connecting) {
        borderColor = QColor("#00e5ff");
    }
    p.setPen(QPen(borderColor, 1.2));
    p.drawPath(trackPath);

    // Thumb
    const qreal margin = 3.0;
    const qreal thumbDiameter = h - 2.0 * margin;
    const qreal startX = margin;
    const qreal endX = w - margin - thumbDiameter;
    const qreal currentX = startX + m_offset * (endX - startX);
    const qreal currentY = margin;

    QRectF thumbRect(currentX, currentY, thumbDiameter, thumbDiameter);

    QColor thumbColor = isChecked() ? QColor("#ffffff") : QColor("#56657a");
    if (m_connecting) {
        thumbColor = QColor("#e0f7fa");
    }

    p.setPen(Qt::NoPen);
    p.setBrush(thumbColor);
    p.drawEllipse(thumbRect);

    // If connecting, draw subtle mini spinner around thumb
    if (m_connecting) {
        p.setPen(QPen(QColor("#00838f"), 2.0));
        QRectF arcRect = thumbRect.adjusted(3, 3, -3, -3);
        p.drawArc(arcRect, m_pulseStep * 16, 120 * 16);
    }
}

