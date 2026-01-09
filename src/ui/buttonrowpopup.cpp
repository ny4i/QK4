#include "buttonrowpopup.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QVBoxLayout>

namespace {
// Indicator bar/triangle color (matches BandPopupWidget)
const QColor IndicatorColor(85, 85, 85); // #555555

// Layout constants (same as BandPopupWidget)
const int ButtonWidth = 70;
const int ButtonHeight = 44;
const int ButtonSpacing = 8;
const int ContentMargin = 12; // Margin around button content
const int TriangleWidth = 24;
const int TriangleHeight = 12;
const int BottomStripHeight = 8; // Gray strip at bottom of popup

// Shadow constants
const int ShadowRadius = 16;               // Drop shadow blur radius
const int ShadowOffsetX = 2;               // Shadow horizontal offset
const int ShadowOffsetY = 4;               // Shadow vertical offset
const int ShadowMargin = ShadowRadius + 4; // Extra space around popup for shadow

// Colors
const char *AmberColor = "#FFB000";
} // namespace

// ============================================================================
// RxMenuButton Implementation
// ============================================================================

RxMenuButton::RxMenuButton(const QString &primaryText, const QString &alternateText, QWidget *parent)
    : QWidget(parent), m_primaryText(primaryText), m_alternateText(alternateText) {
    setFixedSize(ButtonWidth, ButtonHeight);
    setCursor(Qt::PointingHandCursor);
}

void RxMenuButton::setPrimaryText(const QString &text) {
    if (m_primaryText != text) {
        m_primaryText = text;
        update();
    }
}

void RxMenuButton::setAlternateText(const QString &text) {
    if (m_alternateText != text) {
        m_alternateText = text;
        update();
    }
}

void RxMenuButton::setHasAlternateFunction(bool has) {
    if (m_hasAlternateFunction != has) {
        m_hasAlternateFunction = has;
        update();
    }
}

void RxMenuButton::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background - subtle gradient
    QLinearGradient grad(0, 0, 0, height());
    if (m_hovered) {
        grad.setColorAt(0, QColor(90, 90, 90));
        grad.setColorAt(0.4, QColor(74, 74, 74));
        grad.setColorAt(0.6, QColor(69, 69, 69));
        grad.setColorAt(1, QColor(58, 58, 58));
    } else {
        grad.setColorAt(0, QColor(74, 74, 74));
        grad.setColorAt(0.4, QColor(58, 58, 58));
        grad.setColorAt(0.6, QColor(53, 53, 53));
        grad.setColorAt(1, QColor(42, 42, 42));
    }
    painter.setBrush(grad);
    painter.setPen(QPen(QColor(96, 96, 96), 2));
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 5, 5);

    // Check if we have alternate text
    bool hasAlternate = !m_alternateText.isEmpty();

    if (hasAlternate) {
        // Dual-line mode: Primary text (white) - top
        QFont primaryFont = font();
        primaryFont.setPixelSize(12);
        primaryFont.setBold(false);
        painter.setFont(primaryFont);
        painter.setPen(Qt::white);

        QRect primaryRect(0, 4, width(), height() / 2 - 2);
        painter.drawText(primaryRect, Qt::AlignCenter, m_primaryText);

        // Alternate text - bottom (amber if has alternate function, white if just label)
        QFont altFont = font();
        altFont.setPixelSize(10);
        altFont.setBold(false);
        painter.setFont(altFont);
        painter.setPen(m_hasAlternateFunction ? QColor(AmberColor) : Qt::white);

        QRect altRect(0, height() / 2, width(), height() / 2 - 4);
        painter.drawText(altRect, Qt::AlignCenter, m_alternateText);
    } else {
        // Single-line mode: Center the primary text
        QFont primaryFont = font();
        primaryFont.setPixelSize(12);
        primaryFont.setBold(true);
        painter.setFont(primaryFont);
        painter.setPen(Qt::white);

        painter.drawText(rect(), Qt::AlignCenter, m_primaryText);
    }
}

void RxMenuButton::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    } else if (event->button() == Qt::RightButton) {
        emit rightClicked();
    }
}

void RxMenuButton::enterEvent(QEnterEvent *event) {
    Q_UNUSED(event)
    m_hovered = true;
    update();
}

void RxMenuButton::leaveEvent(QEvent *event) {
    Q_UNUSED(event)
    m_hovered = false;
    update();
}

// ============================================================================
// ButtonRowPopup Implementation
// ============================================================================

ButtonRowPopup::ButtonRowPopup(QWidget *parent) : QWidget(parent), m_triangleXOffset(0) {
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::StrongFocus);

    setupUi();
}

void ButtonRowPopup::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    // Margins include shadow space on all sides, plus gray strip + triangle at bottom
    int leftMargin = ShadowMargin + ContentMargin;
    int topMargin = ShadowMargin + ContentMargin;
    int rightMargin = ShadowMargin + ContentMargin;
    int bottomMargin = ShadowMargin + ContentMargin + BottomStripHeight + TriangleHeight;
    mainLayout->setContentsMargins(leftMargin, topMargin, rightMargin, bottomMargin);
    mainLayout->setSpacing(0);

    // Single row of 7 buttons
    auto *rowLayout = new QHBoxLayout();
    rowLayout->setSpacing(ButtonSpacing);

    for (int i = 0; i < 7; ++i) {
        auto *btn = new RxMenuButton(QString::number(i + 1), QString(), this);
        btn->setProperty("buttonIndex", i);

        connect(btn, &RxMenuButton::clicked, this, [this, i]() { emit buttonClicked(i); });
        connect(btn, &RxMenuButton::rightClicked, this, [this, i]() { emit buttonRightClicked(i); });

        m_buttons.append(btn);
        rowLayout->addWidget(btn);
    }

    mainLayout->addLayout(rowLayout);

    // Calculate size (content + shadow margins on all sides)
    int contentWidth = 7 * ButtonWidth + 6 * ButtonSpacing + 2 * ContentMargin;
    int contentHeight = ButtonHeight + 2 * ContentMargin + BottomStripHeight + TriangleHeight;
    int totalWidth = contentWidth + 2 * ShadowMargin;
    int totalHeight = contentHeight + 2 * ShadowMargin;
    setFixedSize(totalWidth, totalHeight);
}

void ButtonRowPopup::setButtonLabels(const QStringList &labels) {
    for (int i = 0; i < qMin(labels.size(), m_buttons.size()); ++i) {
        m_buttons[i]->setPrimaryText(labels[i]);
        m_buttons[i]->setAlternateText(QString()); // Clear alternate
    }
}

void ButtonRowPopup::setButtonLabel(int index, const QString &primary, const QString &alternate,
                                    bool hasAlternateFunction) {
    if (index >= 0 && index < m_buttons.size()) {
        m_buttons[index]->setPrimaryText(primary);
        m_buttons[index]->setAlternateText(alternate);
        m_buttons[index]->setHasAlternateFunction(hasAlternateFunction);
    }
}

QString ButtonRowPopup::buttonLabel(int index) const {
    if (index >= 0 && index < m_buttons.size()) {
        return m_buttons[index]->primaryText();
    }
    return QString();
}

QString ButtonRowPopup::buttonAlternateLabel(int index) const {
    if (index >= 0 && index < m_buttons.size()) {
        return m_buttons[index]->alternateText();
    }
    return QString();
}

void ButtonRowPopup::showAboveButton(QWidget *triggerButton) {
    if (!triggerButton)
        return;

    // Ensure geometry is realized before positioning
    adjustSize();

    // Get the button bar (parent of trigger button) for centering
    QWidget *buttonBar = triggerButton->parentWidget();
    if (!buttonBar)
        buttonBar = triggerButton; // Fallback to button itself

    // Get positions
    QPoint barGlobal = buttonBar->mapToGlobal(QPoint(0, 0));
    QPoint btnGlobal = triggerButton->mapToGlobal(QPoint(0, 0));
    int barCenterX = barGlobal.x() + buttonBar->width() / 2;
    int btnCenterX = btnGlobal.x() + triggerButton->width() / 2;

    // Content dimensions (for positioning relative to visual content, not shadow)
    int contentWidth = 7 * ButtonWidth + 6 * ButtonSpacing + 2 * ContentMargin;
    int contentHeight = ButtonHeight + 2 * ContentMargin + BottomStripHeight + TriangleHeight;

    // Center content area above the button bar (account for shadow margin offset)
    int popupX = barCenterX - contentWidth / 2 - ShadowMargin;
    int popupY = btnGlobal.y() - contentHeight - ShadowMargin;

    // Calculate triangle offset to point at the trigger button
    int contentCenterX = popupX + ShadowMargin + contentWidth / 2;
    m_triangleXOffset = btnCenterX - contentCenterX;

    // Ensure popup stays on screen (adjust position, recalculate triangle)
    QRect screenGeom = QApplication::primaryScreen()->availableGeometry();
    if (popupX < screenGeom.left() - ShadowMargin) {
        popupX = screenGeom.left() - ShadowMargin;
        // Recalculate triangle offset with new popup position
        contentCenterX = popupX + ShadowMargin + contentWidth / 2;
        m_triangleXOffset = btnCenterX - contentCenterX;
    } else if (popupX + width() > screenGeom.right() + ShadowMargin) {
        popupX = screenGeom.right() + ShadowMargin - width();
        // Recalculate triangle offset with new popup position
        contentCenterX = popupX + ShadowMargin + contentWidth / 2;
        m_triangleXOffset = btnCenterX - contentCenterX;
    }

    move(popupX, popupY);
    show();
    raise();
    setFocus();
    update();
}

void ButtonRowPopup::hidePopup() {
    hide();
    // closed() signal is emitted by hideEvent()
}

void ButtonRowPopup::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    emit closed();
}

void ButtonRowPopup::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Content area dimensions (inside shadow margins)
    int contentWidth = 7 * ButtonWidth + 6 * ButtonSpacing + 2 * ContentMargin;
    int contentHeight = ButtonHeight + 2 * ContentMargin + BottomStripHeight;

    // Content rect position (offset by shadow margin)
    QRect contentRect(ShadowMargin, ShadowMargin, contentWidth, contentHeight);

    // Draw drop shadow (multiple layers for soft blur effect)
    painter.setPen(Qt::NoPen);
    const int shadowLayers = 8;
    for (int i = shadowLayers; i > 0; --i) {
        int blur = i * 2;
        int alpha = 12 + (shadowLayers - i) * 3; // Darker toward center
        QRect shadowRect = contentRect.adjusted(-blur, -blur, blur, blur);
        shadowRect.translate(ShadowOffsetX, ShadowOffsetY);
        painter.setBrush(QColor(0, 0, 0, alpha));
        painter.drawRoundedRect(shadowRect, 8 + blur / 2, 8 + blur / 2);
    }

    // Main popup background
    painter.setBrush(QColor(30, 30, 30));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(contentRect, 8, 8);

    // Gray bottom strip (inside the content rect, at bottom)
    QRect stripRect(contentRect.left(), contentRect.bottom() - BottomStripHeight + 1, contentRect.width(),
                    BottomStripHeight);
    painter.fillRect(stripRect, IndicatorColor);

    // Triangle pointing down from center of bottom strip
    painter.setPen(Qt::NoPen);
    painter.setBrush(IndicatorColor);
    int triangleX = contentRect.center().x() + m_triangleXOffset;
    int triangleTop = contentRect.bottom() + 1;
    QPainterPath triangle;
    triangle.moveTo(triangleX - TriangleWidth / 2, triangleTop);
    triangle.lineTo(triangleX + TriangleWidth / 2, triangleTop);
    triangle.lineTo(triangleX, triangleTop + TriangleHeight);
    triangle.closeSubpath();
    painter.drawPath(triangle);
}

void ButtonRowPopup::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        hidePopup();
    } else {
        QWidget::keyPressEvent(event);
    }
}
