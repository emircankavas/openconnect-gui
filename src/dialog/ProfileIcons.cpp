/*
 * ProfileIcons: 20 vector icons for VPN profiles
 */

#include "ProfileIcons.h"
#include <QPixmap>
#include <cmath>

static const char* icon_names[ProfileIcons::COUNT] = {
    "Kalkan (Güvenlik)",          // 0
    "Şirket Binası (Kurumsal)",    // 1
    "Dünya (Global / VPN)",        // 2
    "Şimşek (Hızlı Bağlantı)",     // 3
    "Sunucu (Veri Merkezi)",       // 4
    "Kilit (Gizlilik)",            // 5
    "Bulut (Cloud)",               // 6
    "Roket (Turbo)",               // 7
    "Anahtar (Erişim)",            // 8
    "Veritabanı (Storage)",        // 9
    "Wifi / Kablosuz Ağ",         // 10
    "Uçak (Seyahat / Roaming)",    // 11
    "Ev (Evden Çalışma)",          // 12
    "Dizüstü Bilgisayar (İş)",     // 13
    "Terminal (Geliştirici)",      // 14
    "Radar / Anten",               // 15
    "Göz (Gizli Mod)",             // 16
    "Yıldız (Favori)",             // 17
    "Nabız (Sağlık / İzleme)",     // 18
    "Sonsuzluk (Sınırsız Trafik)"  // 19
};

QString ProfileIcons::name(int index)
{
    if (index >= 0 && index < COUNT) {
        return QString::fromUtf8(icon_names[index]);
    }
    return QString::fromUtf8(icon_names[0]);
}

void ProfileIcons::paint(QPainter& p, int index, const QRectF& r, const QColor& color)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);

    const qreal cx = r.center().x();
    const qreal cy = r.center().y();
    const qreal s = std::min(r.width(), r.height()) / 24.0; // scale factor based on 24x24 base

    p.setPen(QPen(color, 1.8 * s, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    switch (index) {
    case 0: { // Kalkan
        QPainterPath shield;
        shield.moveTo(cx, cy - 9 * s);
        shield.lineTo(cx + 8 * s, cy - 5 * s);
        shield.quadTo(cx + 8 * s, cy + 4 * s, cx, cy + 10 * s);
        shield.quadTo(cx - 8 * s, cy + 4 * s, cx - 8 * s, cy - 5 * s);
        shield.closeSubpath();
        p.drawPath(shield);

        QPainterPath check;
        check.moveTo(cx - 3 * s, cy);
        check.lineTo(cx - 0.5 * s, cy + 2.5 * s);
        check.lineTo(cx + 3.5 * s, cy - 2.5 * s);
        p.drawPath(check);
        break;
    }
    case 1: { // Şirket Binası
        p.drawRect(QRectF(cx - 8 * s, cy - 10 * s, 16 * s, 20 * s));
        p.setPen(QPen(color, 1.2 * s));
        for (int y = -7; y <= 3; y += 4) {
            p.drawLine(QPointF(cx - 5 * s, cy + y * s), QPointF(cx - 2 * s, cy + y * s));
            p.drawLine(QPointF(cx + 2 * s, cy + y * s), QPointF(cx + 5 * s, cy + y * s));
        }
        p.drawRect(QRectF(cx - 2.5 * s, cy + 6 * s, 5 * s, 4 * s));
        break;
    }
    case 2: { // Dünya
        p.drawEllipse(QPointF(cx, cy), 9 * s, 9 * s);
        p.drawEllipse(QPointF(cx, cy), 4.5 * s, 9 * s);
        p.drawLine(QPointF(cx - 9 * s, cy), QPointF(cx + 9 * s, cy));
        break;
    }
    case 3: { // Şimşek
        QPainterPath bolt;
        bolt.moveTo(cx + 1 * s, cy - 10 * s);
        bolt.lineTo(cx - 5 * s, cy);
        bolt.lineTo(cx, cy);
        bolt.lineTo(cx - 1 * s, cy + 10 * s);
        bolt.lineTo(cx + 5 * s, cy);
        bolt.lineTo(cx, cy);
        bolt.closeSubpath();
        p.setBrush(color);
        p.drawPath(bolt);
        break;
    }
    case 4: { // Sunucu
        p.drawRoundedRect(QRectF(cx - 8 * s, cy - 9 * s, 16 * s, 7 * s), 2 * s, 2 * s);
        p.drawRoundedRect(QRectF(cx - 8 * s, cy + 2 * s, 16 * s, 7 * s), 2 * s, 2 * s);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(QPointF(cx - 4.5 * s, cy - 5.5 * s), 1 * s, 1 * s);
        p.drawEllipse(QPointF(cx - 4.5 * s, cy + 5.5 * s), 1 * s, 1 * s);
        p.setPen(QPen(color, 1.4 * s));
        p.drawLine(QPointF(cx + 1 * s, cy - 5.5 * s), QPointF(cx + 5 * s, cy - 5.5 * s));
        p.drawLine(QPointF(cx + 1 * s, cy + 5.5 * s), QPointF(cx + 5 * s, cy + 5.5 * s));
        break;
    }
    case 5: { // Kilit
        p.drawRoundedRect(QRectF(cx - 6 * s, cy - 2 * s, 12 * s, 10 * s), 2 * s, 2 * s);
        QPainterPath shackle;
        shackle.arcMoveTo(QRectF(cx - 4 * s, cy - 8 * s, 8 * s, 9 * s), 0);
        shackle.arcTo(QRectF(cx - 4 * s, cy - 8 * s, 8 * s, 9 * s), 0, 180);
        p.drawPath(shackle);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(QPointF(cx, cy + 2 * s), 1.3 * s, 1.3 * s);
        p.setPen(QPen(color, 1.4 * s));
        p.drawLine(QPointF(cx, cy + 3.3 * s), QPointF(cx, cy + 5.5 * s));
        break;
    }
    case 6: { // Bulut
        QPainterPath cloud;
        cloud.moveTo(cx - 6 * s, cy + 5 * s);
        cloud.lineTo(cx + 6 * s, cy + 5 * s);
        cloud.arcTo(QRectF(cx + 3 * s, cy, 6 * s, 5 * s), -90, 180);
        cloud.arcTo(QRectF(cx - 3 * s, cy - 7 * s, 8 * s, 8 * s), 0, 180);
        cloud.arcTo(QRectF(cx - 9 * s, cy - 1 * s, 6 * s, 6 * s), 90, 180);
        cloud.closeSubpath();
        p.drawPath(cloud);
        break;
    }
    case 7: { // Roket
        QPainterPath rocket;
        rocket.moveTo(cx, cy - 9 * s);
        rocket.quadTo(cx + 5 * s, cy - 3 * s, cx + 4 * s, cy + 5 * s);
        rocket.lineTo(cx - 4 * s, cy + 5 * s);
        rocket.quadTo(cx - 5 * s, cy - 3 * s, cx, cy - 9 * s);
        p.drawPath(rocket);
        // Fins
        p.drawLine(QPointF(cx - 4 * s, cy + 2 * s), QPointF(cx - 7 * s, cy + 7 * s));
        p.drawLine(QPointF(cx + 4 * s, cy + 2 * s), QPointF(cx + 7 * s, cy + 7 * s));
        // Flame
        p.drawLine(QPointF(cx, cy + 5 * s), QPointF(cx, cy + 9 * s));
        break;
    }
    case 8: { // Anahtar
        p.drawEllipse(QPointF(cx - 4 * s, cy - 4 * s), 4 * s, 4 * s);
        p.drawLine(QPointF(cx - 1 * s, cy - 1 * s), QPointF(cx + 7 * s, cy + 7 * s));
        p.drawLine(QPointF(cx + 5 * s, cy + 5 * s), QPointF(cx + 7 * s, cy + 3 * s));
        p.drawLine(QPointF(cx + 3 * s, cy + 3 * s), QPointF(cx + 5 * s, cy + 1 * s));
        break;
    }
    case 9: { // Veritabanı
        p.drawEllipse(QPointF(cx, cy - 6 * s), 8 * s, 3 * s);
        p.drawLine(QPointF(cx - 8 * s, cy - 6 * s), QPointF(cx - 8 * s, cy + 6 * s));
        p.drawLine(QPointF(cx + 8 * s, cy - 6 * s), QPointF(cx + 8 * s, cy + 6 * s));
        QPainterPath arc1, arc2;
        arc1.arcMoveTo(QRectF(cx - 8 * s, cy - 3 * s, 16 * s, 6 * s), 180);
        arc1.arcTo(QRectF(cx - 8 * s, cy - 3 * s, 16 * s, 6 * s), 180, 180);
        p.drawPath(arc1);
        arc2.arcMoveTo(QRectF(cx - 8 * s, cy + 3 * s, 16 * s, 6 * s), 180);
        arc2.arcTo(QRectF(cx - 8 * s, cy + 3 * s, 16 * s, 6 * s), 180, 180);
        p.drawPath(arc2);
        break;
    }
    case 10: { // Wifi
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(QPointF(cx, cy + 6 * s), 1.5 * s, 1.5 * s);
        p.setPen(QPen(color, 1.8 * s, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        QPainterPath a1, a2;
        a1.arcMoveTo(QRectF(cx - 5 * s, cy + 1 * s, 10 * s, 9 * s), 45);
        a1.arcTo(QRectF(cx - 5 * s, cy + 1 * s, 10 * s, 9 * s), 45, 90);
        p.drawPath(a1);
        a2.arcMoveTo(QRectF(cx - 8.5 * s, cy - 3.5 * s, 17 * s, 15 * s), 45);
        a2.arcTo(QRectF(cx - 8.5 * s, cy - 3.5 * s, 17 * s, 15 * s), 45, 90);
        p.drawPath(a2);
        break;
    }
    case 11: { // Uçak
        QPainterPath plane;
        plane.moveTo(cx, cy - 9 * s);
        plane.lineTo(cx + 2 * s, cy - 2 * s);
        plane.lineTo(cx + 9 * s, cy + 2 * s);
        plane.lineTo(cx + 9 * s, cy + 4 * s);
        plane.lineTo(cx + 2 * s, cy + 2 * s);
        plane.lineTo(cx + 2 * s, cy + 6 * s);
        plane.lineTo(cx + 5 * s, cy + 8 * s);
        plane.lineTo(cx + 5 * s, cy + 9.5 * s);
        plane.lineTo(cx, cy + 8.5 * s);
        plane.lineTo(cx - 5 * s, cy + 9.5 * s);
        plane.lineTo(cx - 5 * s, cy + 8 * s);
        plane.lineTo(cx - 2 * s, cy + 6 * s);
        plane.lineTo(cx - 2 * s, cy + 2 * s);
        plane.lineTo(cx - 9 * s, cy + 4 * s);
        plane.lineTo(cx - 9 * s, cy + 2 * s);
        plane.lineTo(cx - 2 * s, cy - 2 * s);
        plane.closeSubpath();
        p.drawPath(plane);
        break;
    }
    case 12: { // Ev
        QPainterPath roof;
        roof.moveTo(cx - 9 * s, cy - 1 * s);
        roof.lineTo(cx, cy - 9 * s);
        roof.lineTo(cx + 9 * s, cy - 1 * s);
        p.drawPath(roof);
        p.drawRect(QRectF(cx - 7 * s, cy - 1 * s, 14 * s, 9 * s));
        p.drawRect(QRectF(cx - 2 * s, cy + 3 * s, 4 * s, 5 * s));
        break;
    }
    case 13: { // Laptop
        p.drawRoundedRect(QRectF(cx - 7 * s, cy - 7 * s, 14 * s, 10 * s), 1.5 * s, 1.5 * s);
        p.drawLine(QPointF(cx - 9.5 * s, cy + 6 * s), QPointF(cx + 9.5 * s, cy + 6 * s));
        p.drawLine(QPointF(cx - 3 * s, cy + 6 * s), QPointF(cx + 3 * s, cy + 6 * s));
        break;
    }
    case 14: { // Terminal
        p.drawRoundedRect(QRectF(cx - 8 * s, cy - 7 * s, 16 * s, 14 * s), 2 * s, 2 * s);
        QPainterPath prompt;
        prompt.moveTo(cx - 5 * s, cy - 3 * s);
        prompt.lineTo(cx - 2.5 * s, cy);
        prompt.lineTo(cx - 5 * s, cy + 3 * s);
        p.drawPath(prompt);
        p.drawLine(QPointF(cx - 1 * s, cy + 3 * s), QPointF(cx + 4 * s, cy + 3 * s));
        break;
    }
    case 15: { // Radar
        p.drawEllipse(QPointF(cx, cy), 8 * s, 8 * s);
        p.drawEllipse(QPointF(cx, cy), 4 * s, 4 * s);
        p.drawLine(QPointF(cx, cy), QPointF(cx + 6 * s, cy - 5 * s));
        break;
    }
    case 16: { // Göz / Gizlilik
        QPainterPath eye;
        eye.moveTo(cx - 9 * s, cy);
        eye.quadTo(cx, cy - 6 * s, cx + 9 * s, cy);
        eye.quadTo(cx, cy + 6 * s, cx - 9 * s, cy);
        p.drawPath(eye);
        p.drawEllipse(QPointF(cx, cy), 3 * s, 3 * s);
        break;
    }
    case 17: { // Yıldız
        QPainterPath star;
        for (int i = 0; i < 5; ++i) {
            qreal a1 = -M_PI / 2.0 + i * 2.0 * M_PI / 5.0;
            qreal a2 = a1 + M_PI / 5.0;
            qreal r1 = 8.5 * s;
            qreal r2 = 4.0 * s;
            if (i == 0) {
                star.moveTo(cx + r1 * std::cos(a1), cy + r1 * std::sin(a1));
            } else {
                star.lineTo(cx + r1 * std::cos(a1), cy + r1 * std::sin(a1));
            }
            star.lineTo(cx + r2 * std::cos(a2), cy + r2 * std::sin(a2));
        }
        star.closeSubpath();
        p.drawPath(star);
        break;
    }
    case 18: { // Nabız
        QPainterPath pulse;
        pulse.moveTo(cx - 9 * s, cy);
        pulse.lineTo(cx - 4 * s, cy);
        pulse.lineTo(cx - 2 * s, cy - 7 * s);
        pulse.lineTo(cx + 1 * s, cy + 8 * s);
        pulse.lineTo(cx + 3 * s, cy - 3 * s);
        pulse.lineTo(cx + 5 * s, cy);
        pulse.lineTo(cx + 9 * s, cy);
        p.drawPath(pulse);
        break;
    }
    case 19: // Sonsuzluk
    default: {
        QPainterPath inf;
        inf.moveTo(cx, cy);
        inf.cubicTo(cx - 4 * s, cy - 6 * s, cx - 9 * s, cy - 6 * s, cx - 9 * s, cy);
        inf.cubicTo(cx - 9 * s, cy + 6 * s, cx - 4 * s, cy + 6 * s, cx, cy);
        inf.cubicTo(cx + 4 * s, cy - 6 * s, cx + 9 * s, cy - 6 * s, cx + 9 * s, cy);
        inf.cubicTo(cx + 9 * s, cy + 6 * s, cx + 4 * s, cy + 6 * s, cx, cy);
        p.drawPath(inf);
        break;
    }
    }

    p.restore();
}

QIcon ProfileIcons::icon(int index, const QSize& size, const QColor& color)
{
    QPixmap pix(size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    paint(p, index, QRectF(0, 0, size.width(), size.height()), color);
    return QIcon(pix);
}

void ProfileIcons::populateComboBox(QComboBox* cb)
{
    if (!cb) return;
    cb->clear();
    cb->setIconSize(QSize(20, 20));
    for (int i = 0; i < COUNT; ++i) {
        cb->addItem(icon(i, QSize(20, 20), QColor("#00d2d3")), name(i), i);
    }
}

