#include "buttonrowpopup.h"
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
const int Margin = 12;
const int TriangleWidth = 24;
const int TriangleHeight = 12;
const int BottomStripHeight = 8; // Gray strip at bottom of popup
} // namespace

ButtonRowPopup::ButtonRowPopup(QWidget *parent) : QWidget(parent), m_triangleXOffset(0) {
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::StrongFocus);

    setupUi();
}

void ButtonRowPopup::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    // Bottom margin includes space for the gray strip + triangle extending below
    mainLayout->setContentsMargins(Margin, Margin, Margin, Margin + BottomStripHeight + TriangleHeight);
    mainLayout->setSpacing(0);

    // Single row of 7 buttons
    auto *rowLayout = new QHBoxLayout();
    rowLayout->setSpacing(ButtonSpacing);

    for (int i = 0; i < 7; ++i) {
        auto *btn = new QPushButton(QString::number(i + 1), this);
        btn->setFixedSize(ButtonWidth, ButtonHeight);
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setProperty("buttonIndex", i);
        btn->setStyleSheet(normalButtonStyle());

        connect(btn, &QPushButton::clicked, this, &ButtonRowPopup::onButtonClicked);

        m_buttons.append(btn);
        rowLayout->addWidget(btn);
    }

    mainLayout->addLayout(rowLayout);

    // Calculate size (1 row instead of 2)
    int totalWidth = 7 * ButtonWidth + 6 * ButtonSpacing + 2 * Margin;
    int totalHeight = ButtonHeight + 2 * Margin + BottomStripHeight + TriangleHeight;
    setFixedSize(totalWidth, totalHeight);
}

QString ButtonRowPopup::normalButtonStyle() {
    return R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4a4a4a,
                stop:0.4 #3a3a3a,
                stop:0.6 #353535,
                stop:1 #2a2a2a);
            color: #FFFFFF;
            border: 1px solid #606060;
            border-radius: 5px;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #5a5a5a,
                stop:0.4 #4a4a4a,
                stop:0.6 #454545,
                stop:1 #3a3a3a);
            border: 1px solid #808080;
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2a2a2a,
                stop:0.4 #353535,
                stop:0.6 #3a3a3a,
                stop:1 #4a4a4a);
        }
    )";
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

    // Show off-screen first to ensure geometry is realized (fixes first-show positioning bug)
    move(-10000, -10000);
    show();

    // Get the button bar (parent of trigger button) for centering
    QWidget *buttonBar = triggerButton->parentWidget();
    if (!buttonBar)
        buttonBar = triggerButton; // Fallback to button itself

    // Get positions
    QPoint barGlobal = buttonBar->mapToGlobal(QPoint(0, 0));
    QPoint btnGlobal = triggerButton->mapToGlobal(QPoint(0, 0));
    int barCenterX = barGlobal.x() + buttonBar->width() / 2;
    int btnCenterX = btnGlobal.x() + triggerButton->width() / 2;

    // Center popup above the button bar
    int popupX = barCenterX - width() / 2;
    int popupY = btnGlobal.y() - height();

    // Calculate triangle offset to point at the trigger button
    // Triangle position = button center relative to popup center
    int popupCenterX = popupX + width() / 2;
    m_triangleXOffset = btnCenterX - popupCenterX;

    // Ensure popup stays on screen (adjust position, recalculate triangle)
    QRect screenGeom = QApplication::primaryScreen()->availableGeometry();
    if (popupX < screenGeom.left()) {
        popupX = screenGeom.left();
        // Recalculate triangle offset with new popup position
        popupCenterX = popupX + width() / 2;
        m_triangleXOffset = btnCenterX - popupCenterX;
    } else if (popupX + width() > screenGeom.right()) {
        popupX = screenGeom.right() - width();
        // Recalculate triangle offset with new popup position
        popupCenterX = popupX + width() / 2;
        m_triangleXOffset = btnCenterX - popupCenterX;
    }

    move(popupX, popupY);
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

    // Main popup area (everything except the triangle that extends below)
    int mainHeight = height() - TriangleHeight;
    QRect mainRect(0, 0, width(), mainHeight);

    // Fill main background with rounded corners (matches FnPopupWidget)
    painter.setBrush(QColor(30, 30, 30));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(mainRect, 8, 8);

    // Gray bottom strip (inside the main rect, at bottom)
    QRect stripRect(0, mainHeight - BottomStripHeight, width(), BottomStripHeight);
    painter.fillRect(stripRect, IndicatorColor);

    // Triangle pointing down from center of bottom strip
    painter.setPen(Qt::NoPen);
    painter.setBrush(IndicatorColor);
    int triangleX = width() / 2 + m_triangleXOffset;
    QPainterPath triangle;
    triangle.moveTo(triangleX - TriangleWidth / 2, mainHeight);
    triangle.lineTo(triangleX + TriangleWidth / 2, mainHeight);
    triangle.lineTo(triangleX, height());
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
