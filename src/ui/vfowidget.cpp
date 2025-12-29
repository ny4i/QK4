#include "vfowidget.h"
#include "smeterwidget.h"
#include "../dsp/minipanwidget.h"
#include <QMouseEvent>

// K4 Color constants
namespace K4Colors {
const QString Background = "#1a1a1a";
const QString DarkBackground = "#0d0d0d";
const QString VfoAAmber = "#FFB000";
const QString VfoBCyan = "#00BFFF";
const QString InactiveGray = "#666666";
const QString TextWhite = "#FFFFFF";
} // namespace K4Colors

VFOWidget::VFOWidget(VFOType type, QWidget *parent)
    : QWidget(parent), m_type(type), m_primaryColor(type == VFO_A ? K4Colors::VfoAAmber : K4Colors::VfoBCyan),
      m_inactiveColor(K4Colors::InactiveGray) {
    setupUi();
}

void VFOWidget::setupUi() {
    setStyleSheet(QString("background-color: %1;").arg(K4Colors::Background));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 4, 8, 4);
    mainLayout->setSpacing(2);

    // Row 1: Frequency (aligned with S-meter)
    auto *freqRow = new QHBoxLayout();
    m_frequencyLabel = new QLabel("---.---.---", this);
    m_frequencyLabel->setStyleSheet(
        QString("color: %1; font-size: 32px; font-weight: bold; font-family: 'Courier New', monospace;")
            .arg(K4Colors::TextWhite));
    m_frequencyLabel->setFixedHeight(40);

    if (m_type == VFO_A) {
        // VFO A: frequency left-aligned with S-meter
        freqRow->addWidget(m_frequencyLabel);
        freqRow->addStretch();
    } else {
        // VFO B: frequency right-aligned with S-meter
        freqRow->addStretch();
        freqRow->addWidget(m_frequencyLabel);
    }
    mainLayout->addLayout(freqRow);

    // Stacked widget for normal content vs mini-pan
    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->setFixedHeight(60); // Height for S-meter + features rows

    // Page 0: Normal content (S-meter + features)
    m_normalContent = new QWidget(m_stackedWidget);
    auto *normalLayout = new QVBoxLayout(m_normalContent);
    normalLayout->setContentsMargins(0, 0, 0, 0);
    normalLayout->setSpacing(2);

    // Row 2: S-Meter (compressed width, aligned to outer edge)
    auto *sMeterRow = new QHBoxLayout();
    sMeterRow->setContentsMargins(0, 0, 0, 0);
    m_sMeter = new SMeterWidget(m_normalContent);
    m_sMeter->setColor(QColor(m_primaryColor));

    if (m_type == VFO_A) {
        // VFO A: S-meter left, space right (for filter indicator)
        sMeterRow->addWidget(m_sMeter);
        sMeterRow->addStretch();
    } else {
        // VFO B: space left (for filter indicator), S-meter right
        sMeterRow->addStretch();
        sMeterRow->addWidget(m_sMeter);
    }
    normalLayout->addLayout(sMeterRow);

    // Row 3: AGC, PRE, ATT, NB, NR labels (aligned with S-meter)
    auto *featuresRow = new QHBoxLayout();

    m_agcLabel = new QLabel("AGC-S", m_normalContent);
    m_agcLabel->setStyleSheet("color: #999999; font-size: 11px; font-weight: bold;");

    m_preampLabel = new QLabel("PRE", m_normalContent);
    m_preampLabel->setStyleSheet("color: #999999; font-size: 11px; font-weight: bold;");

    m_attLabel = new QLabel("ATT", m_normalContent);
    m_attLabel->setStyleSheet("color: #999999; font-size: 11px;");

    m_nbLabel = new QLabel("NB", m_normalContent);
    m_nbLabel->setStyleSheet("color: #999999; font-size: 11px;");

    m_nrLabel = new QLabel("NR", m_normalContent);
    m_nrLabel->setStyleSheet("color: #999999; font-size: 11px;");

    m_ntchLabel = new QLabel("NTCH", m_normalContent);
    m_ntchLabel->setStyleSheet("color: #999999; font-size: 11px;");

    if (m_type == VFO_A) {
        // VFO A: features left-aligned with S-meter
        featuresRow->addWidget(m_agcLabel);
        featuresRow->addWidget(m_preampLabel);
        featuresRow->addWidget(m_attLabel);
        featuresRow->addWidget(m_nbLabel);
        featuresRow->addWidget(m_nrLabel);
        featuresRow->addWidget(m_ntchLabel);
        featuresRow->addStretch();
    } else {
        // VFO B: features right-aligned with S-meter
        featuresRow->addStretch();
        featuresRow->addWidget(m_agcLabel);
        featuresRow->addWidget(m_preampLabel);
        featuresRow->addWidget(m_attLabel);
        featuresRow->addWidget(m_nbLabel);
        featuresRow->addWidget(m_nrLabel);
        featuresRow->addWidget(m_ntchLabel);
    }
    normalLayout->addLayout(featuresRow);

    m_stackedWidget->addWidget(m_normalContent); // Index 0

    // Page 1: Mini-Pan widget
    m_miniPan = new MiniPanWidget(m_stackedWidget);
    m_stackedWidget->addWidget(m_miniPan); // Index 1

    // Connect mini-pan click to show normal view
    connect(m_miniPan, &MiniPanWidget::clicked, this, &VFOWidget::showNormal);

    mainLayout->addWidget(m_stackedWidget);

    // Install event filter for click-to-toggle on normal content
    m_normalContent->installEventFilter(this);
}

bool VFOWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_normalContent && event->type() == QEvent::MouseButtonPress) {
        emit normalContentClicked();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void VFOWidget::setFrequency(const QString &formatted) {
    m_frequencyLabel->setText(formatted);
}

void VFOWidget::setSMeterValue(double value) {
    m_sMeter->setValue(value);
}

void VFOWidget::setAGC(const QString &mode) {
    m_agcLabel->setText(mode);
    // AGC is always shown, color indicates active state
    bool active = !mode.contains("-") || mode == "AGC-F" || mode == "AGC-S" || mode == "AGC-M";
    m_agcLabel->setStyleSheet(
        QString("color: %1; font-size: 11px; font-weight: bold;").arg(active ? "#FFFFFF" : "#999999"));
}

void VFOWidget::setPreamp(bool on) {
    m_preampLabel->setStyleSheet(
        QString("color: %1; font-size: 11px; font-weight: bold;").arg(on ? "#FFFFFF" : "#999999"));
}

void VFOWidget::setAtt(bool on) {
    m_attLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(on ? "#FFFFFF" : "#999999"));
}

void VFOWidget::setNB(bool on) {
    m_nbLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(on ? "#FFFFFF" : "#999999"));
}

void VFOWidget::setNR(bool on) {
    m_nrLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(on ? "#FFFFFF" : "#999999"));
}

void VFOWidget::setNotch(bool autoEnabled, bool manualEnabled) {
    QString text = "NTCH";
    bool active = autoEnabled || manualEnabled;

    if (autoEnabled && manualEnabled) {
        text = "NTCH-A/M";
    } else if (autoEnabled) {
        text = "NTCH-A";
    } else if (manualEnabled) {
        text = "NTCH-M";
    }

    m_ntchLabel->setText(text);
    m_ntchLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(active ? "#FFFFFF" : "#999999"));
}

void VFOWidget::updateMiniPan(const QByteArray &data) {
    m_miniPan->updateSpectrum(data);
}

void VFOWidget::showMiniPan() {
    m_stackedWidget->setCurrentIndex(1);
}

void VFOWidget::showNormal() {
    m_stackedWidget->setCurrentIndex(0);
}

bool VFOWidget::isMiniPanVisible() const {
    return m_stackedWidget->currentIndex() == 1;
}
