#include "buttonrowpopup.h"
#include "k4styles.h"
#include <QPainter>
#include <QPainterPath>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QHideEvent>
#include <QApplication>
#include <QScreen>

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
} // namespace

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
        auto *btn = new QPushButton(QString::number(i + 1), this);
        btn->setFixedSize(ButtonWidth, ButtonHeight);
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setProperty("buttonIndex", i);
        btn->setStyleSheet(K4Styles::popupButtonNormal());

        connect(btn, &QPushButton::clicked, this, &ButtonRowPopup::onButtonClicked);

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
        m_buttons[i]->setText(labels[i]);
    }
}

void ButtonRowPopup::setButtonLabel(int index, const QString &label) {
    if (index >= 0 && index < m_buttons.size()) {
        m_buttons[index]->setText(label);
    }
}

QString ButtonRowPopup::buttonLabel(int index) const {
    if (index >= 0 && index < m_buttons.size()) {
        return m_buttons[index]->text();
    }
    return QString();
}

void ButtonRowPopup::onButtonClicked() {
    auto *btn = qobject_cast<QPushButton *>(sender());
    if (btn) {
        int index = btn->property("buttonIndex").toInt();
        emit buttonClicked(index);
        hidePopup();
    }
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
