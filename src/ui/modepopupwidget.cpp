#include "modepopupwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QKeyEvent>
#include <QHideEvent>
#include <QApplication>
#include <QScreen>

namespace {
// Indicator bar/triangle color (matches other popups)
const QColor IndicatorColor(85, 85, 85); // #555555

// Layout constants
const int ButtonWidth = 70;
const int ButtonHeight = 44;
const int ButtonSpacing = 8;
const int RowSpacing = 2;
const int Margin = 12;
const int TriangleWidth = 24;
const int TriangleHeight = 12;
const int BottomStripHeight = 8;

// Mode codes (from K4 protocol)
const int MODE_LSB = 1;
const int MODE_USB = 2;
const int MODE_CW = 3;
const int MODE_FM = 4;
const int MODE_AM = 5;
const int MODE_DATA = 6;
const int MODE_CW_R = 7;
const int MODE_DATA_R = 9;

// Data sub-mode codes
const int DT_DATA_A = 0;
const int DT_AFSK_A = 1;
const int DT_FSK_D = 2;
const int DT_PSK_D = 3;
} // namespace

ModePopupWidget::ModePopupWidget(QWidget *parent) : QWidget(parent), m_triangleXOffset(0) {
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::StrongFocus);

    setupUi();
}

void ModePopupWidget::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(Margin, Margin, Margin, Margin + BottomStripHeight + TriangleHeight);
    mainLayout->setSpacing(RowSpacing);

    // Create 2x4 grid
    auto *gridLayout = new QGridLayout();
    gridLayout->setSpacing(ButtonSpacing);

    // Row 1: CW, SSB, DATA, AFSK
    m_cwBtn = createModeButton("CW");
    m_cwBtn->setProperty("modeType", "CW");
    gridLayout->addWidget(m_cwBtn, 0, 0);

    m_ssbBtn = createModeButton("USB"); // Will show LSB or USB based on current mode
    m_ssbBtn->setProperty("modeType", "SSB");
    gridLayout->addWidget(m_ssbBtn, 0, 1);

    m_dataBtn = createModeButton("DATA");
    m_dataBtn->setProperty("modeType", "DATA");
    gridLayout->addWidget(m_dataBtn, 0, 2);

    m_afskBtn = createModeButton("AFSK");
    m_afskBtn->setProperty("modeType", "AFSK");
    gridLayout->addWidget(m_afskBtn, 0, 3);

    // Row 2: AM, FM, PSK, FSK
    m_amBtn = createModeButton("AM");
    m_amBtn->setProperty("modeType", "AM");
    gridLayout->addWidget(m_amBtn, 1, 0);

    m_fmBtn = createModeButton("FM");
    m_fmBtn->setProperty("modeType", "FM");
    gridLayout->addWidget(m_fmBtn, 1, 1);

    m_pskBtn = createModeButton("PSK");
    m_pskBtn->setProperty("modeType", "PSK");
    gridLayout->addWidget(m_pskBtn, 1, 2);

    m_fskBtn = createModeButton("FSK");
    m_fskBtn->setProperty("modeType", "FSK");
    gridLayout->addWidget(m_fskBtn, 1, 3);

    mainLayout->addLayout(gridLayout);

    // Store button references
    m_buttonMap["CW"] = m_cwBtn;
    m_buttonMap["SSB"] = m_ssbBtn;
    m_buttonMap["DATA"] = m_dataBtn;
    m_buttonMap["AFSK"] = m_afskBtn;
    m_buttonMap["AM"] = m_amBtn;
    m_buttonMap["FM"] = m_fmBtn;
    m_buttonMap["PSK"] = m_pskBtn;
    m_buttonMap["FSK"] = m_fskBtn;

    // Calculate size (4 columns, 2 rows)
    int totalWidth = 4 * ButtonWidth + 3 * ButtonSpacing + 2 * Margin;
    int totalHeight = 2 * ButtonHeight + RowSpacing + 2 * Margin + BottomStripHeight + TriangleHeight;
    setFixedSize(totalWidth, totalHeight);

    updateButtonStyles();
}

QPushButton *ModePopupWidget::createModeButton(const QString &text) {
    auto *btn = new QPushButton(text, this);
    btn->setFixedSize(ButtonWidth, ButtonHeight);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setCursor(Qt::PointingHandCursor);

    connect(btn, &QPushButton::clicked, this, &ModePopupWidget::onModeButtonClicked);

    return btn;
}

QString ModePopupWidget::normalButtonStyle() {
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

QString ModePopupWidget::selectedButtonStyle() {
    return R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #E0E0E0,
                stop:0.4 #D0D0D0,
                stop:0.6 #C8C8C8,
                stop:1 #B8B8B8);
            color: #333333;
            border: 1px solid #AAAAAA;
            border-radius: 5px;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #F0F0F0,
                stop:0.4 #E0E0E0,
                stop:0.6 #D8D8D8,
                stop:1 #C8C8C8);
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #B8B8B8,
                stop:0.4 #C8C8C8,
                stop:0.6 #D0D0D0,
                stop:1 #E0E0E0);
        }
    )";
}

void ModePopupWidget::updateButtonStyles() {
    // Update SSB button text based on current mode or band default
    if (m_currentMode == MODE_LSB) {
        m_ssbBtn->setText("LSB");
    } else if (m_currentMode == MODE_USB) {
        m_ssbBtn->setText("USB");
    } else {
        // Not in SSB mode - show band-appropriate default
        m_ssbBtn->setText(bandDefaultSideband() == MODE_LSB ? "LSB" : "USB");
    }

    // Reset all buttons to normal style
    for (auto it = m_buttonMap.begin(); it != m_buttonMap.end(); ++it) {
        it.value()->setStyleSheet(normalButtonStyle());
    }

    // Highlight the current mode button
    switch (m_currentMode) {
    case MODE_CW:
    case MODE_CW_R:
        m_cwBtn->setStyleSheet(selectedButtonStyle());
        break;
    case MODE_LSB:
    case MODE_USB:
        m_ssbBtn->setStyleSheet(selectedButtonStyle());
        break;
    case MODE_AM:
        m_amBtn->setStyleSheet(selectedButtonStyle());
        break;
    case MODE_FM:
        m_fmBtn->setStyleSheet(selectedButtonStyle());
        break;
    case MODE_DATA:
    case MODE_DATA_R:
        // Highlight based on data sub-mode
        switch (m_currentDataSubMode) {
        case DT_DATA_A:
            m_dataBtn->setStyleSheet(selectedButtonStyle());
            break;
        case DT_AFSK_A:
            m_afskBtn->setStyleSheet(selectedButtonStyle());
            break;
        case DT_FSK_D:
            m_fskBtn->setStyleSheet(selectedButtonStyle());
            break;
        case DT_PSK_D:
            m_pskBtn->setStyleSheet(selectedButtonStyle());
            break;
        }
        break;
    }
}

void ModePopupWidget::setCurrentMode(int modeCode) {
    m_currentMode = modeCode;
    updateButtonStyles();
}

void ModePopupWidget::setCurrentDataSubMode(int subMode) {
    m_currentDataSubMode = subMode;
    updateButtonStyles();
}

void ModePopupWidget::setBSetEnabled(bool enabled) {
    m_bSetEnabled = enabled;
}

void ModePopupWidget::setFrequency(quint64 freqHz) {
    m_frequency = freqHz;
    updateButtonStyles();
}

int ModePopupWidget::bandDefaultSideband() const {
    // Amateur radio convention: LSB below 10 MHz, USB at 10 MHz and above
    // 160m (1.8), 80m (3.5), 60m (5), 40m (7) = LSB
    // 30m (10), 20m (14), 17m (18), 15m (21), 12m (24), 10m (28), 6m (50) = USB
    return (m_frequency < 10000000) ? MODE_LSB : MODE_USB;
}

void ModePopupWidget::onModeButtonClicked() {
    auto *btn = qobject_cast<QPushButton *>(sender());
    if (!btn)
        return;

    QString modeType = btn->property("modeType").toString();
    QString cmd;
    QString prefix = m_bSetEnabled ? "MD$" : "MD";
    QString dtPrefix = m_bSetEnabled ? "DT$" : "DT";

    if (modeType == "CW") {
        cmd = prefix + "3;";
    } else if (modeType == "SSB") {
        // If already in SSB mode, toggle between LSB and USB
        // Otherwise, switch to band-appropriate sideband (LSB <10MHz, USB >=10MHz)
        if (m_currentMode == MODE_LSB) {
            cmd = prefix + "2;"; // Toggle to USB
        } else if (m_currentMode == MODE_USB) {
            cmd = prefix + "1;"; // Toggle to LSB
        } else {
            // Not in SSB - use band-appropriate default
            cmd = prefix + QString::number(bandDefaultSideband()) + ";";
        }
    } else if (modeType == "DATA") {
        cmd = prefix + "6;" + dtPrefix + "0;"; // DATA mode + DATA-A sub-mode
    } else if (modeType == "AFSK") {
        cmd = prefix + "6;" + dtPrefix + "1;"; // DATA mode + AFSK-A sub-mode
    } else if (modeType == "AM") {
        cmd = prefix + "5;";
    } else if (modeType == "FM") {
        cmd = prefix + "4;";
    } else if (modeType == "PSK") {
        cmd = prefix + "6;" + dtPrefix + "3;"; // DATA mode + PSK-D sub-mode
    } else if (modeType == "FSK") {
        cmd = prefix + "6;" + dtPrefix + "2;"; // DATA mode + FSK-D sub-mode
    }

    if (!cmd.isEmpty()) {
        emit modeSelected(cmd);
    }
    hidePopup();
}

void ModePopupWidget::showAboveWidget(QWidget *referenceWidget, QWidget *arrowTarget) {
    if (!referenceWidget)
        return;

    // Ensure geometry is realized before positioning
    adjustSize();

    // Get the reference widget's parent (button bar) for centering
    QWidget *buttonBar = referenceWidget->parentWidget();
    if (!buttonBar)
        buttonBar = referenceWidget;

    // Use arrowTarget if provided, otherwise default to referenceWidget
    QWidget *triangleTarget = arrowTarget ? arrowTarget : referenceWidget;

    QPoint barGlobal = buttonBar->mapToGlobal(QPoint(0, 0));
    QPoint refGlobal = referenceWidget->mapToGlobal(QPoint(0, 0));
    QPoint targetGlobal = triangleTarget->mapToGlobal(QPoint(0, 0));
    int barCenterX = barGlobal.x() + buttonBar->width() / 2;
    int targetCenterX = targetGlobal.x() + triangleTarget->width() / 2;

    // Center popup above the button bar
    int popupX = barCenterX - width() / 2;
    int popupY = refGlobal.y() - height();

    // Calculate triangle offset to point at the arrow target widget
    int popupCenterX = popupX + width() / 2;
    m_triangleXOffset = targetCenterX - popupCenterX;

    // Ensure popup stays on screen
    QRect screenGeom = QApplication::primaryScreen()->availableGeometry();
    if (popupX < screenGeom.left()) {
        popupX = screenGeom.left();
        popupCenterX = popupX + width() / 2;
        m_triangleXOffset = targetCenterX - popupCenterX;
    } else if (popupX + width() > screenGeom.right()) {
        popupX = screenGeom.right() - width();
        popupCenterX = popupX + width() / 2;
        m_triangleXOffset = targetCenterX - popupCenterX;
    }

    move(popupX, popupY);
    show();
    raise();
    setFocus();
    update();
}

void ModePopupWidget::hidePopup() {
    hide();
}

void ModePopupWidget::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    emit closed();
}

void ModePopupWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Main popup area (everything except the triangle)
    int mainHeight = height() - TriangleHeight;
    QRect mainRect(0, 0, width(), mainHeight);

    // Fill main background with rounded corners (matches FnPopupWidget)
    painter.setBrush(QColor(30, 30, 30));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(mainRect, 8, 8);

    // Gray bottom strip
    QRect stripRect(0, mainHeight - BottomStripHeight, width(), BottomStripHeight);
    painter.fillRect(stripRect, IndicatorColor);

    // Triangle pointing down
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

void ModePopupWidget::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        hidePopup();
    } else {
        QWidget::keyPressEvent(event);
    }
}
