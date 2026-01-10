#include "vforowwidget.h"
#include "k4styles.h"
#include <QHBoxLayout>
#include <QResizeEvent>

VfoRowWidget::VfoRowWidget(QWidget *parent) : QWidget(parent) {
    setupWidgets();
    // Set fixed height to match content (A/B squares + mode labels)
    setFixedHeight(55);
}

void VfoRowWidget::setupWidgets() {
    // No layout manager - we use absolute positioning
    // All containers are children of this widget

    // === VFO A Container (square + mode label) ===
    m_vfoAContainer = new QWidget(this);
    m_vfoAContainer->setFixedWidth(45);
    auto *vfoAColumn = new QVBoxLayout(m_vfoAContainer);
    vfoAColumn->setContentsMargins(0, 0, 0, 0);
    vfoAColumn->setSpacing(2);

    m_vfoASquare = new QLabel("A", m_vfoAContainer);
    m_vfoASquare->setFixedSize(30, 30);
    m_vfoASquare->setAlignment(Qt::AlignCenter);
    m_vfoASquare->setCursor(Qt::PointingHandCursor);
    m_vfoASquare->setStyleSheet(
        QString("background-color: %1; color: %2; font-size: 16px; font-weight: bold; border-radius: 4px;")
            .arg(K4Styles::Colors::VfoBCyan, K4Styles::Colors::DarkBackground));
    vfoAColumn->addWidget(m_vfoASquare, 0, Qt::AlignHCenter);

    m_modeALabel = new QLabel("USB", m_vfoAContainer);
    m_modeALabel->setFixedWidth(45);
    m_modeALabel->setAlignment(Qt::AlignCenter);
    m_modeALabel->setCursor(Qt::PointingHandCursor);
    m_modeALabel->setStyleSheet(
        QString("color: %1; font-size: 11px; font-weight: bold;").arg(K4Styles::Colors::TextWhite));
    vfoAColumn->addWidget(m_modeALabel, 0, Qt::AlignHCenter);

    // === TX Container (TEST label + triangles + TX) ===
    m_txContainer = new QWidget(this);
    auto *txVLayout = new QVBoxLayout(m_txContainer);
    txVLayout->setContentsMargins(0, 0, 0, 0);
    txVLayout->setSpacing(0);

    // TEST indicator - hidden by default
    m_testLabel = new QLabel("TEST", m_txContainer);
    m_testLabel->setAlignment(Qt::AlignCenter);
    m_testLabel->setStyleSheet("color: #FF0000; font-size: 14px; font-weight: bold;");
    m_testLabel->setVisible(false);
    txVLayout->addWidget(m_testLabel);

    // TX row (triangles + TX label)
    auto *txIndicatorRow = new QHBoxLayout();
    txIndicatorRow->setSpacing(0);

    m_txTriangle = new QLabel(QString::fromUtf8("\u25C0"), m_txContainer); // ◀
    m_txTriangle->setFixedSize(24, 24);
    m_txTriangle->setAlignment(Qt::AlignCenter);
    m_txTriangle->setStyleSheet(QString("color: %1; font-size: 18px;").arg(K4Styles::Colors::VfoAAmber));
    txIndicatorRow->addWidget(m_txTriangle);

    m_txIndicator = new QLabel("TX", m_txContainer);
    m_txIndicator->setStyleSheet(
        QString("color: %1; font-size: 18px; font-weight: bold;").arg(K4Styles::Colors::VfoAAmber));
    txIndicatorRow->addWidget(m_txIndicator);

    m_txTriangleB = new QLabel("", m_txContainer); // Empty by default
    m_txTriangleB->setFixedSize(24, 24);
    m_txTriangleB->setAlignment(Qt::AlignCenter);
    m_txTriangleB->setStyleSheet(QString("color: %1; font-size: 18px;").arg(K4Styles::Colors::VfoAAmber));
    txIndicatorRow->addWidget(m_txTriangleB);

    txVLayout->addLayout(txIndicatorRow);

    // Adjust size to fit content
    m_txContainer->adjustSize();

    // === VFO B Container (square + mode label) ===
    m_vfoBContainer = new QWidget(this);
    m_vfoBContainer->setFixedWidth(45);
    auto *vfoBColumn = new QVBoxLayout(m_vfoBContainer);
    vfoBColumn->setContentsMargins(0, 0, 0, 0);
    vfoBColumn->setSpacing(2);

    m_vfoBSquare = new QLabel("B", m_vfoBContainer);
    m_vfoBSquare->setFixedSize(30, 30);
    m_vfoBSquare->setAlignment(Qt::AlignCenter);
    m_vfoBSquare->setCursor(Qt::PointingHandCursor);
    m_vfoBSquare->setStyleSheet(
        QString("background-color: %1; color: %2; font-size: 16px; font-weight: bold; border-radius: 4px;")
            .arg(K4Styles::Colors::AgcGreen, K4Styles::Colors::DarkBackground));
    vfoBColumn->addWidget(m_vfoBSquare, 0, Qt::AlignHCenter);

    m_modeBLabel = new QLabel("USB", m_vfoBContainer);
    m_modeBLabel->setFixedWidth(45);
    m_modeBLabel->setAlignment(Qt::AlignCenter);
    m_modeBLabel->setCursor(Qt::PointingHandCursor);
    m_modeBLabel->setStyleSheet(
        QString("color: %1; font-size: 11px; font-weight: bold;").arg(K4Styles::Colors::TextWhite));
    vfoBColumn->addWidget(m_modeBLabel, 0, Qt::AlignHCenter);

    // === SUB/DIV Container ===
    m_subDivContainer = new QWidget(this);
    auto *subDivStack = new QVBoxLayout(m_subDivContainer);
    subDivStack->setSpacing(4);
    subDivStack->setContentsMargins(0, 0, 0, 0);

    m_subLabel = new QLabel("SUB", m_subDivContainer);
    m_subLabel->setAlignment(Qt::AlignCenter);
    m_subLabel->setFixedSize(36, 14);
    m_subLabel->setStyleSheet("background-color: #444444;"
                              "color: #888888;"
                              "font-size: 9px;"
                              "font-weight: bold;"
                              "border-radius: 2px;");
    subDivStack->addWidget(m_subLabel);

    m_divLabel = new QLabel("DIV", m_subDivContainer);
    m_divLabel->setAlignment(Qt::AlignCenter);
    m_divLabel->setFixedSize(36, 14);
    m_divLabel->setStyleSheet("background-color: #444444;"
                              "color: #888888;"
                              "font-size: 9px;"
                              "font-weight: bold;"
                              "border-radius: 2px;");
    subDivStack->addWidget(m_divLabel);

    m_subDivContainer->adjustSize();
}

void VfoRowWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    positionWidgets();
}

void VfoRowWidget::positionWidgets() {
    int w = width();
    int centerX = w / 2;
    int y = 0; // Top of row

    // TX container - centered at widget center
    m_txContainer->adjustSize();
    int txWidth = m_txContainer->width();
    m_txContainer->move(centerX - txWidth / 2, y);

    // A container - left of TX with gap
    int gap = 15;
    m_vfoAContainer->move(centerX - txWidth / 2 - gap - m_vfoAContainer->width(), y);

    // B container - right of TX with gap (symmetric with A)
    m_vfoBContainer->move(centerX + txWidth / 2 + gap, y);

    // SUB/DIV - to right of B (doesn't affect centering)
    m_subDivContainer->move(m_vfoBContainer->x() + m_vfoBContainer->width() + 10, y);
}
