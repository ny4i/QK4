#include "dualcontrolbutton.h"
#include "k4styles.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLinearGradient>

DualControlButton::DualControlButton(QWidget *parent) : QWidget(parent) {
    setFixedSize(K4Styles::Dimensions::MenuBarButtonWidth, K4Styles::Dimensions::ButtonHeightLarge);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
}

void DualControlButton::setPrimaryLabel(const QString &label) {
    m_primaryLabel = label;
    update();
}

void DualControlButton::setPrimaryValue(const QString &value) {
    m_primaryValue = value;
    update();
}

void DualControlButton::setAlternateLabel(const QString &label) {
    m_alternateLabel = label;
    update();
}

void DualControlButton::setAlternateValue(const QString &value) {
    m_alternateValue = value;
    update();
}

void DualControlButton::setContext(Context context) {
    m_context = context;
    update();
}

void DualControlButton::setShowIndicator(bool show) {
    m_showIndicator = show;
    update();
}

void DualControlButton::swapFunctions() {
    // Swap labels
    QString tempLabel = m_primaryLabel;
    m_primaryLabel = m_alternateLabel;
    m_alternateLabel = tempLabel;

    // Swap values
    QString tempValue = m_primaryValue;
    m_primaryValue = m_alternateValue;
    m_alternateValue = tempValue;

    update();
}

QColor DualControlButton::contextColor() const {
    switch (m_context) {
    case Global:
        return QColor(K4Styles::Colors::AccentAmber);
    case MainRx:
        return QColor(K4Styles::Colors::VfoACyan);
    case SubRx:
        return QColor(K4Styles::Colors::VfoBGreen);
    }
    return QColor(K4Styles::Colors::VfoACyan);
}

void DualControlButton::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int barWidth = 5; // Context indicator bar width
    int radius = 4;   // Subtle rounded corners
    int margin = 1;

    // Background with subtle gradient
    QLinearGradient bgGradient(0, 0, 0, h);
    if (m_isHovered) {
        bgGradient.setColorAt(0.0, QColor(K4Styles::Colors::HoverMid2));
        bgGradient.setColorAt(1.0, QColor(K4Styles::Colors::GradientMid2));
    } else {
        bgGradient.setColorAt(0.0, QColor(K4Styles::Colors::GradientMid1));
        bgGradient.setColorAt(1.0, QColor(K4Styles::Colors::GradientBottom));
    }

    // Main button area (always reserve space for indicator bar so button doesn't shift)
    int buttonLeft = barWidth + margin + 2;
    QRectF buttonRect(buttonLeft, margin, w - buttonLeft - margin, h - margin * 2);
    QPainterPath buttonPath;
    buttonPath.addRoundedRect(buttonRect, radius, radius);

    painter.fillPath(buttonPath, bgGradient);

    // Border (slightly brighter when indicator is shown)
    QColor borderColor =
        m_showIndicator ? QColor(K4Styles::Colors::BorderHover) : QColor(K4Styles::Colors::BorderNormal);
    painter.setPen(QPen(borderColor, 2));
    painter.drawPath(buttonPath);

    // Context indicator bar on the left (only if active in group)
    if (m_showIndicator) {
        QRectF barRect(margin, margin + 4, barWidth, h - margin * 2 - 8);
        QPainterPath barPath;
        barPath.addRoundedRect(barRect, 2, 2);
        painter.fillPath(barPath, contextColor());
    }

    // Text setup
    QFont labelFont = font();
    labelFont.setPointSize(K4Styles::Dimensions::FontSizeLarge);
    labelFont.setBold(true);

    QFont valueFont = font();
    valueFont.setPointSize(K4Styles::Dimensions::FontSizeButton);
    valueFont.setBold(true);

    QFont altFont = font();
    altFont.setPointSize(K4Styles::Dimensions::FontSizeNormal);

    int textLeft = buttonLeft + 6;
    int textRight = w - 6;

    // Primary label (left side, white) - e.g., "WPM"
    painter.setFont(labelFont);
    painter.setPen(QColor(K4Styles::Colors::TextWhite));
    painter.drawText(textLeft, 18, m_primaryLabel);

    // Primary value (right side, white) - e.g., "15"
    painter.setFont(valueFont);
    QFontMetrics fm(valueFont);
    int valueWidth = fm.horizontalAdvance(m_primaryValue);
    painter.drawText(textRight - valueWidth, 18, m_primaryValue);

    // Alternate label and value (bottom, yellow/amber) - e.g., "PTCH 600"
    painter.setFont(altFont);
    painter.setPen(QColor(K4Styles::Colors::AccentAmber));

    QString altText = m_alternateLabel;
    if (!m_alternateValue.isEmpty()) {
        altText += " " + m_alternateValue;
    }
    painter.drawText(textLeft, h - 10, altText);
}

void DualControlButton::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (!m_showIndicator) {
            // First click on inactive button: just activate, don't swap
            emit becameActive();
        } else {
            // Already active: swap functions
            swapFunctions();
            emit swapped();
        }
        emit clicked();
        event->accept();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void DualControlButton::wheelEvent(QWheelEvent *event) {
    // Only respond to scroll if this button has the indicator (is active)
    if (m_showIndicator) {
        int delta = event->angleDelta().y() > 0 ? 1 : -1;
        emit valueScrolled(delta);
        event->accept();
    } else {
        // Make this button active first, then respond to scroll
        emit becameActive();
        int delta = event->angleDelta().y() > 0 ? 1 : -1;
        emit valueScrolled(delta);
        event->accept();
    }
}

void DualControlButton::enterEvent(QEnterEvent *event) {
    m_isHovered = true;
    update();
    QWidget::enterEvent(event);
}

void DualControlButton::leaveEvent(QEvent *event) {
    m_isHovered = false;
    update();
    QWidget::leaveEvent(event);
}
