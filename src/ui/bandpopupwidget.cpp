#include "bandpopupwidget.h"
#include "k4styles.h"
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QHideEvent>
#include <QApplication>

namespace {
// Layout constants
const int ButtonWidth = 70;
const int ButtonHeight = 44;
const int ButtonSpacing = 8;
const int RowSpacing = 2;
const int ContentMargin = 12; // Margin around button content
const int TriangleWidth = 24;
const int TriangleHeight = 12;
const int BottomStripHeight = 8; // Gray strip at bottom of popup

// K4 Band Number mapping (BN command)
// Band number -> button label
const QMap<int, QString> BandNumToName = {
    {0, "1.8"}, // 160m
    {1, "3.5"}, // 80m
    {2, "5"},   // 60m
    {3, "7"},   // 40m
    {4, "10"},  // 30m
    {5, "14"},  // 20m
    {6, "18"},  // 17m
    {7, "21"},  // 15m
    {8, "24"},  // 12m
    {9, "28"},  // 10m
    {10, "50"}, // 6m
    // 16-25 are transverter bands, all map to "XVTR"
};

// Button label -> band number
const QMap<QString, int> BandNameToNum = {
    {"1.8", 0}, {"3.5", 1}, {"5", 2},  {"7", 3},  {"10", 4},  {"14", 5},
    {"18", 6},  {"21", 7},  {"24", 8}, {"28", 9}, {"50", 10}, {"XVTR", 16}, // First transverter band (16-25 range)
};
} // namespace

BandPopupWidget::BandPopupWidget(QWidget *parent)
    : QWidget(parent), m_selectedBand("14") // Default to 20m band
      ,
      m_triangleXOffset(0) {
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::StrongFocus);

    setupUi();
}

void BandPopupWidget::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    // Margins include shadow space on all sides, plus gray strip + triangle at bottom
    int leftMargin = K4Styles::Dimensions::ShadowMargin + ContentMargin;
    int topMargin = K4Styles::Dimensions::ShadowMargin + ContentMargin;
    int rightMargin = K4Styles::Dimensions::ShadowMargin + ContentMargin;
    int bottomMargin = K4Styles::Dimensions::ShadowMargin + ContentMargin + BottomStripHeight + TriangleHeight;
    mainLayout->setContentsMargins(leftMargin, topMargin, rightMargin, bottomMargin);
    mainLayout->setSpacing(RowSpacing);

    // Row 1: 1.8, 3.5, 7, 14, 21, 28, MEM
    QStringList row1Bands = {"1.8", "3.5", "7", "14", "21", "28", "MEM"};
    auto *row1Layout = new QHBoxLayout();
    row1Layout->setSpacing(ButtonSpacing);
    for (const QString &band : row1Bands) {
        QPushButton *btn = createBandButton(band);
        m_row1Buttons.append(btn);
        m_buttonMap[band] = btn;
        row1Layout->addWidget(btn);
    }
    mainLayout->addLayout(row1Layout);

    // Row 2: GEN, 5, 10, 18, 24, 50, XVTR
    QStringList row2Bands = {"GEN", "5", "10", "18", "24", "50", "XVTR"};
    auto *row2Layout = new QHBoxLayout();
    row2Layout->setSpacing(ButtonSpacing);
    for (const QString &band : row2Bands) {
        QPushButton *btn = createBandButton(band);
        m_row2Buttons.append(btn);
        m_buttonMap[band] = btn;
        row2Layout->addWidget(btn);
    }
    mainLayout->addLayout(row2Layout);

    // Update styles to show selected band
    updateButtonStyles();

    // Calculate size (content + shadow margins on all sides)
    int contentWidth = 7 * ButtonWidth + 6 * ButtonSpacing + 2 * ContentMargin;
    int contentHeight = 2 * ButtonHeight + RowSpacing + 2 * ContentMargin + BottomStripHeight + TriangleHeight;
    int totalWidth = contentWidth + 2 * K4Styles::Dimensions::ShadowMargin;
    int totalHeight = contentHeight + 2 * K4Styles::Dimensions::ShadowMargin;
    setFixedSize(totalWidth, totalHeight);
}

QPushButton *BandPopupWidget::createBandButton(const QString &text) {
    QPushButton *btn = new QPushButton(text, this);
    btn->setFixedSize(ButtonWidth, ButtonHeight);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setProperty("bandName", text);

    connect(btn, &QPushButton::clicked, this, &BandPopupWidget::onBandButtonClicked);

    return btn;
}

void BandPopupWidget::updateButtonStyles() {
    for (auto it = m_buttonMap.begin(); it != m_buttonMap.end(); ++it) {
        if (it.key() == m_selectedBand) {
            it.value()->setStyleSheet(K4Styles::popupButtonSelected());
        } else {
            it.value()->setStyleSheet(K4Styles::popupButtonNormal());
        }
    }
}

void BandPopupWidget::setSelectedBand(const QString &bandName) {
    if (m_buttonMap.contains(bandName)) {
        m_selectedBand = bandName;
        updateButtonStyles();
    }
}

void BandPopupWidget::onBandButtonClicked() {
    QPushButton *btn = qobject_cast<QPushButton *>(sender());
    if (btn) {
        QString bandName = btn->property("bandName").toString();
        m_selectedBand = bandName;
        updateButtonStyles();
        emit bandSelected(bandName);
        hidePopup();
    }
}

void BandPopupWidget::showAboveButton(QWidget *triggerButton) {
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
    int contentHeight = 2 * ButtonHeight + RowSpacing + 2 * ContentMargin + BottomStripHeight + TriangleHeight;

    // Center content area above the button bar (account for shadow margin offset)
    int popupX = barCenterX - contentWidth / 2 - K4Styles::Dimensions::ShadowMargin;
    int popupY = btnGlobal.y() - contentHeight - K4Styles::Dimensions::ShadowMargin;

    // Calculate triangle offset to point at the trigger button
    // Triangle position = button center relative to content center
    int contentCenterX = popupX + K4Styles::Dimensions::ShadowMargin + contentWidth / 2;
    m_triangleXOffset = btnCenterX - contentCenterX;

    // Ensure popup stays on screen (adjust position, recalculate triangle)
    QRect screenGeom = QApplication::primaryScreen()->availableGeometry();
    if (popupX < screenGeom.left() - K4Styles::Dimensions::ShadowMargin) {
        popupX = screenGeom.left() - K4Styles::Dimensions::ShadowMargin;
        // Recalculate triangle offset with new popup position
        contentCenterX = popupX + K4Styles::Dimensions::ShadowMargin + contentWidth / 2;
        m_triangleXOffset = btnCenterX - contentCenterX;
    } else if (popupX + width() > screenGeom.right() + K4Styles::Dimensions::ShadowMargin) {
        popupX = screenGeom.right() + K4Styles::Dimensions::ShadowMargin - width();
        // Recalculate triangle offset with new popup position
        contentCenterX = popupX + K4Styles::Dimensions::ShadowMargin + contentWidth / 2;
        m_triangleXOffset = btnCenterX - contentCenterX;
    }

    move(popupX, popupY);
    show();
    raise();
    setFocus();
    update();
}

void BandPopupWidget::hidePopup() {
    hide();
    // closed() signal is emitted by hideEvent()
}

void BandPopupWidget::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    emit closed();
}

void BandPopupWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Content area dimensions (inside shadow margins)
    int contentWidth = 7 * ButtonWidth + 6 * ButtonSpacing + 2 * ContentMargin;
    int contentHeight = 2 * ButtonHeight + RowSpacing + 2 * ContentMargin + BottomStripHeight;

    // Content rect position (offset by shadow margin)
    QRect contentRect(K4Styles::Dimensions::ShadowMargin, K4Styles::Dimensions::ShadowMargin, contentWidth,
                      contentHeight);

    // Draw drop shadow
    K4Styles::drawDropShadow(painter, contentRect, 8);

    // Main popup background
    painter.setBrush(QColor(30, 30, 30));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(contentRect, 8, 8);

    // Gray bottom strip (inside the content rect, at bottom)
    QRect stripRect(contentRect.left(), contentRect.bottom() - BottomStripHeight + 1, contentRect.width(),
                    BottomStripHeight);
    painter.fillRect(stripRect, QColor(K4Styles::Colors::IndicatorStrip));

    // Triangle pointing down from center of bottom strip
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(K4Styles::Colors::IndicatorStrip));
    int triangleX = contentRect.center().x() + m_triangleXOffset;
    int triangleTop = contentRect.bottom() + 1;
    QPainterPath triangle;
    triangle.moveTo(triangleX - TriangleWidth / 2, triangleTop);
    triangle.lineTo(triangleX + TriangleWidth / 2, triangleTop);
    triangle.lineTo(triangleX, triangleTop + TriangleHeight);
    triangle.closeSubpath();
    painter.drawPath(triangle);
}

void BandPopupWidget::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        hidePopup();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void BandPopupWidget::focusOutEvent(QFocusEvent *event) {
    Q_UNUSED(event)
    // Don't auto-close on focus out - let popup behavior handle it
    // hidePopup();
}

int BandPopupWidget::getBandNumber(const QString &bandName) const {
    return BandNameToNum.value(bandName, -1); // -1 for GEN, MEM, or unknown
}

QString BandPopupWidget::getBandName(int bandNum) const {
    // Transverter bands 16-25 all map to XVTR
    if (bandNum >= 16 && bandNum <= 25) {
        return "XVTR";
    }
    return BandNumToName.value(bandNum, QString());
}

void BandPopupWidget::setSelectedBandByNumber(int bandNum) {
    QString bandName = getBandName(bandNum);
    if (!bandName.isEmpty()) {
        setSelectedBand(bandName);
    }
}
