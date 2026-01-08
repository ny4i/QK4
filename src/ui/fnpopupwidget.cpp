#include "fnpopupwidget.h"
#include "../settings/radiosettings.h"
#include <QApplication>
#include <QHideEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QVBoxLayout>

namespace {
// Layout constants (similar to DisplayPopupWidget)
const int ButtonWidth = 70;
const int ButtonHeight = 44;
const int ButtonSpacing = 8;
const int Margin = 12;
const int TriangleWidth = 24;
const int TriangleHeight = 12;
const int BottomStripHeight = 8;

// Colors
const QColor IndicatorColor(85, 85, 85); // #555555
const char *AmberColor = "#FFB000";
const char *BackgroundColor = "#1E1E1E";
} // namespace

// ============================================================================
// FnMenuButton Implementation
// ============================================================================

FnMenuButton::FnMenuButton(const QString &primaryText, const QString &alternateText, QWidget *parent)
    : QWidget(parent), m_primaryText(primaryText), m_alternateText(alternateText) {
    setFixedSize(ButtonWidth, ButtonHeight);
    setCursor(Qt::PointingHandCursor);
}

void FnMenuButton::setPrimaryText(const QString &text) {
    if (m_primaryText != text) {
        m_primaryText = text;
        update();
    }
}

void FnMenuButton::setAlternateText(const QString &text) {
    if (m_alternateText != text) {
        m_alternateText = text;
        update();
    }
}

void FnMenuButton::paintEvent(QPaintEvent *event) {
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
    painter.setPen(QPen(QColor(96, 96, 96), 1));
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 5, 5);

    // Primary text (white) - top
    QFont primaryFont = font();
    primaryFont.setPixelSize(12);
    primaryFont.setBold(false);
    painter.setFont(primaryFont);
    painter.setPen(Qt::white);

    QRect primaryRect(0, 4, width(), height() / 2 - 2);
    painter.drawText(primaryRect, Qt::AlignCenter, m_primaryText);

    // Alternate text (amber) - bottom (only if not empty)
    if (!m_alternateText.isEmpty()) {
        QFont altFont = font();
        altFont.setPixelSize(10);
        altFont.setBold(false);
        painter.setFont(altFont);
        painter.setPen(QColor(AmberColor));

        QRect altRect(0, height() / 2, width(), height() / 2 - 4);
        painter.drawText(altRect, Qt::AlignCenter, m_alternateText);
    }
}

void FnMenuButton::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    } else if (event->button() == Qt::RightButton) {
        emit rightClicked();
    }
}

void FnMenuButton::enterEvent(QEnterEvent *event) {
    Q_UNUSED(event)
    m_hovered = true;
    update();
}

void FnMenuButton::leaveEvent(QEvent *event) {
    Q_UNUSED(event)
    m_hovered = false;
    update();
}

// ============================================================================
// FnPopupWidget Implementation
// ============================================================================

FnPopupWidget::FnPopupWidget(QWidget *parent) : QWidget(parent) {
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // Calculate size: 7 buttons + margins + spacing
    int totalWidth = 7 * ButtonWidth + 6 * ButtonSpacing + 2 * Margin;
    int totalHeight = ButtonHeight + 2 * Margin + BottomStripHeight + TriangleHeight;
    setFixedSize(totalWidth, totalHeight);

    setupButtons();

    // Install event filter to detect clicks outside
    if (qApp) {
        qApp->installEventFilter(this);
    }

    // Connect to settings changes
    connect(RadioSettings::instance(), &RadioSettings::macrosChanged, this, &FnPopupWidget::updateButtonLabels);
}

void FnPopupWidget::setupButtons() {
    // Create 7 buttons with initial labels
    // Buttons 1-4: Fn.F1/F2, F3/F4, F5/F6, F7/F8
    // Button 5: SCRN CAP / MACROS
    // Button 6: SW LIST / UPDATE
    // Button 7: DXLIST / (empty)

    struct ButtonDef {
        QString primary;
        QString alternate;
        QString primaryId;
        QString alternateId;
    };

    QVector<ButtonDef> buttonDefs = {{"F1", "F2", MacroIds::FnF1, MacroIds::FnF2},
                                     {"F3", "F4", MacroIds::FnF3, MacroIds::FnF4},
                                     {"F5", "F6", MacroIds::FnF5, MacroIds::FnF6},
                                     {"F7", "F8", MacroIds::FnF7, MacroIds::FnF8},
                                     {"SCRN CAP", "MACROS", MacroIds::ScrnCap, MacroIds::Macros},
                                     {"SW LIST", "UPDATE", MacroIds::SwList, MacroIds::Update},
                                     {"DXLIST", "", MacroIds::DxList, ""}};

    for (int i = 0; i < buttonDefs.size(); ++i) {
        auto btn = new FnMenuButton(buttonDefs[i].primary, buttonDefs[i].alternate, this);
        btn->setPrimaryFunctionId(buttonDefs[i].primaryId);
        btn->setAlternateFunctionId(buttonDefs[i].alternateId);

        // Position button
        int x = Margin + i * (ButtonWidth + ButtonSpacing);
        int y = Margin;
        btn->move(x, y);

        // Connect signals
        connect(btn, &FnMenuButton::clicked, this, [this, i]() { onButtonClicked(i); });
        connect(btn, &FnMenuButton::rightClicked, this, [this, i]() { onButtonRightClicked(i); });

        m_buttons.append(btn);
    }

    // Update labels from saved macros
    updateButtonLabels();
}

void FnPopupWidget::updateButtonLabels() {
    auto settings = RadioSettings::instance();

    // Update buttons 1-4 (Fn.F1-F8) with custom labels if defined
    for (int i = 0; i < 4 && i < m_buttons.size(); ++i) {
        auto btn = m_buttons[i];

        // Get macro labels for primary and alternate
        MacroEntry primaryMacro = settings->macro(btn->primaryFunctionId());
        MacroEntry alternateMacro = settings->macro(btn->alternateFunctionId());

        // Use custom label if defined, otherwise default (F1, F2, etc.)
        QString primaryLabel = primaryMacro.label.isEmpty() ? QString("F%1").arg(i * 2 + 1) : primaryMacro.label;
        QString alternateLabel = alternateMacro.label.isEmpty() ? QString("F%1").arg(i * 2 + 2) : alternateMacro.label;

        btn->setPrimaryText(primaryLabel);
        btn->setAlternateText(alternateLabel);
    }
}

void FnPopupWidget::onButtonClicked(int buttonIndex) {
    if (buttonIndex >= 0 && buttonIndex < m_buttons.size()) {
        QString functionId = m_buttons[buttonIndex]->primaryFunctionId();
        if (!functionId.isEmpty()) {
            emit functionTriggered(functionId);
        }
    }
    hidePopup();
}

void FnPopupWidget::onButtonRightClicked(int buttonIndex) {
    if (buttonIndex >= 0 && buttonIndex < m_buttons.size()) {
        QString functionId = m_buttons[buttonIndex]->alternateFunctionId();
        if (!functionId.isEmpty()) {
            emit functionTriggered(functionId);
        }
    }
    hidePopup();
}

void FnPopupWidget::showAboveButton(QWidget *triggerButton) {
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
    int popupCenterX = popupX + width() / 2;
    m_triangleOffset = btnCenterX - popupCenterX;

    // Ensure popup stays on screen (adjust position, recalculate triangle)
    QScreen *screen = QApplication::screenAt(btnGlobal);
    if (screen) {
        QRect screenRect = screen->availableGeometry();
        if (popupX < screenRect.left()) {
            popupX = screenRect.left();
            popupCenterX = popupX + width() / 2;
            m_triangleOffset = btnCenterX - popupCenterX;
        } else if (popupX + width() > screenRect.right()) {
            popupX = screenRect.right() - width();
            popupCenterX = popupX + width() / 2;
            m_triangleOffset = btnCenterX - popupCenterX;
        }
        popupY = qMax(screenRect.top(), popupY);
    }

    move(popupX, popupY);
    raise();
    activateWindow();
}

void FnPopupWidget::hidePopup() {
    hide();
}

void FnPopupWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Main background
    QRect mainRect(0, 0, width(), height() - TriangleHeight);
    painter.setBrush(QColor(BackgroundColor));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(mainRect, 8, 8);

    // Bottom strip (indicator bar)
    QRect stripRect(0, height() - BottomStripHeight - TriangleHeight, width(), BottomStripHeight);
    painter.setBrush(IndicatorColor);
    painter.drawRect(stripRect);

    // Triangle pointing down (centered + offset to point at trigger button)
    QPainterPath triangle;
    int triangleX = width() / 2 + m_triangleOffset; // Center + offset
    int triY = height() - TriangleHeight;
    triangle.moveTo(triangleX - TriangleWidth / 2, triY);
    triangle.lineTo(triangleX + TriangleWidth / 2, triY);
    triangle.lineTo(triangleX, triY + TriangleHeight);
    triangle.closeSubpath();
    painter.fillPath(triangle, IndicatorColor);
}

void FnPopupWidget::hideEvent(QHideEvent *event) {
    Q_UNUSED(event)
    emit closed();
}

bool FnPopupWidget::eventFilter(QObject *watched, QEvent *event) {
    if (isVisible() && event->type() == QEvent::MouseButtonPress) {
        auto mouseEvent = static_cast<QMouseEvent *>(event);
        QPoint globalPos = mouseEvent->globalPosition().toPoint();

        // Check if click is outside this popup
        if (!geometry().contains(globalPos)) {
            hidePopup();
        }
    }
    return QWidget::eventFilter(watched, event);
}
