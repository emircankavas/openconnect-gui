/*
 * Modern animated toggle switch widget for Qt
 */

#pragma once

#include <QAbstractButton>
#include <QColor>
#include <QPropertyAnimation>

class ToggleSwitch : public QAbstractButton {
    Q_OBJECT
    Q_PROPERTY(qreal offset READ offset WRITE setOffset)
    Q_PROPERTY(QColor trackColor READ trackColor WRITE setTrackColor)

public:
    explicit ToggleSwitch(QWidget* parent = nullptr);

    QSize sizeHint() const override;

    qreal offset() const { return m_offset; }
    void setOffset(qreal o);

    QColor trackColor() const { return m_trackColor; }
    void setTrackColor(const QColor& c);

    void setConnecting(bool connecting);
    bool isConnecting() const { return m_connecting; }

    void setCheckedSilent(bool checked);

protected:
    void paintEvent(QPaintEvent* event) override;
    void nextCheckState() override;

private:
    qreal m_offset;
    QColor m_trackColor;
    bool m_connecting;
    QPropertyAnimation* m_animOffset;
    QPropertyAnimation* m_animColor;
    QTimer* m_pulseTimer;
    int m_pulseStep;
};

