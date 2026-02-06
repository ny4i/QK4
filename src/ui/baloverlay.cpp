#include "baloverlay.h"
#include "k4styles.h"
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QFont>

BalOverlay::BalOverlay(QWidget *parent)
    : SideControlOverlay(MainRx, parent) // Cyan indicator bar
{
    setupUi();
}

void BalOverlay::setupUi() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(IndicatorBarWidth + 8, 8, 8, 8);
    layout->setSpacing(2);

    // Title "SUB AF"
    m_titleLabel = new QLabel("SUB AF", this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPixelSize(K4Styles::Dimensions::FontSizeNormal);
    titleFont.setBold(false);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setStyleSheet(QString("color: %1;").arg(K4Styles::Colors::TextWhite));
    layout->addWidget(m_titleLabel);

    // Mode "= NOR"
    m_modeLabel = new QLabel("= NOR", this);
    QFont modeFont = m_modeLabel->font();
    modeFont.setPixelSize(K4Styles::Dimensions::FontSizeMedium);
    modeFont.setBold(true);
    m_modeLabel->setFont(modeFont);
    m_modeLabel->setStyleSheet(QString("color: %1;").arg(K4Styles::Colors::TextWhite));
    layout->addWidget(m_modeLabel);

    // Spacer
    layout->addSpacing(4);

    // Main value
    m_mainLabel = new QLabel("MAIN:  50", this);
    QFont valueFont = m_mainLabel->font();
    valueFont.setPixelSize(K4Styles::Dimensions::FontSizeNormal);
    m_mainLabel->setFont(valueFont);
    m_mainLabel->setStyleSheet(QString("color: %1;").arg(K4Styles::Colors::TextGray));
    layout->addWidget(m_mainLabel);

    // Sub value
    m_subLabel = new QLabel("SUB:   50", this);
    m_subLabel->setFont(valueFont);
    m_subLabel->setStyleSheet(QString("color: %1;").arg(K4Styles::Colors::TextGray));
    layout->addWidget(m_subLabel);

    layout->addStretch();
}

void BalOverlay::setBalanceMode(const QString &mode) {
    m_balanceMode = mode;
    updateDisplay();
}

void BalOverlay::setMainValue(int value) {
    m_mainValue = value;
    updateDisplay();
}

void BalOverlay::setSubValue(int value) {
    m_subValue = value;
    updateDisplay();
}

void BalOverlay::updateDisplay() {
    m_modeLabel->setText(QString("= %1").arg(m_balanceMode));
    m_mainLabel->setText(QString("MAIN:  %1").arg(m_mainValue));
    m_subLabel->setText(QString("SUB:   %1").arg(m_subValue));
}

void BalOverlay::wheelEvent(QWheelEvent *event) {
    // Convert scroll to delta
    int delta = event->angleDelta().y() > 0 ? 1 : -1;
    emit balanceChangeRequested(delta);
    event->accept();
}

void BalOverlay::mousePressEvent(QMouseEvent *event) {
    // Don't close on click - allow adjustment via wheel
    // Click does nothing, user must click the BAL button again to close
    Q_UNUSED(event)
}
