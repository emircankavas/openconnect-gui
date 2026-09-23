/*
 * ProfileIcons: 20 vector icons for VPN profiles
 */

#pragma once

#include <QString>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QComboBox>

class ProfileIcons {
public:
    static constexpr int COUNT = 20;

    static QString name(int index);
    static void paint(QPainter& p, int index, const QRectF& rect, const QColor& color = QColor("#a0b2c6"));
    static QIcon icon(int index, const QSize& size = QSize(22, 22), const QColor& color = QColor("#00c2cb"));
    static void populateComboBox(QComboBox* cb);
};

