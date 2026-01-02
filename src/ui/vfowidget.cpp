#include "vfowidget.h"
#include "smeterwidget.h"
#include "../dsp/minipan_rhi.h"
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
    // Use dynamic height - stacked widget resizes based on active page
    // Use Maximum horizontal policy so it doesn't expand beyond content width
    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    m_stackedWidget->setMaximumWidth(200); // Match mini-pan max width

    // Page 0: Normal content (S-meter + features)
    m_normalContent = new QWidget(m_stackedWidget);
    m_normalContent->setFixedHeight(60); // S-meter + features = 60px
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

    // Page 1: Placeholder for Mini-Pan widget
    // IMPORTANT: MiniPan is created lazily in showMiniPan() to avoid
    // breaking QRhiWidget initialization for other widgets.
    // Having a non-visible QRhiWidget in a QStackedWidget prevents
    // ALL QRhiWidgets in the window from initializing properly.
    m_miniPan = nullptr; // Will be created on first showMiniPan() call

    // Wrap stacked widget in HBox for edge alignment
    // VFO A: left-aligned (stretch on right)
    // VFO B: right-aligned (stretch on left)
    auto *stackedRow = new QHBoxLayout();
    stackedRow->setContentsMargins(0, 0, 0, 0);
    if (m_type == VFO_A) {
        stackedRow->addWidget(m_stackedWidget);
        stackedRow->addStretch(); // Push to left edge
    } else {
        stackedRow->addStretch(); // Push to right edge
        stackedRow->addWidget(m_stackedWidget);
    }
    mainLayout->addLayout(stackedRow);

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
    // Only update if mini-pan exists and is visible
    if (m_miniPan && m_stackedWidget->currentIndex() == 1) {
        m_miniPan->updateSpectrum(data);
    }
}

void VFOWidget::showMiniPan() {
    // Lazily create MiniPan on first show
    // This avoids breaking QRhiWidget initialization - see note in setupUi()
    if (!m_miniPan) {
        m_miniPan = new MiniPanRhiWidget(m_stackedWidget);
        m_stackedWidget->addWidget(m_miniPan); // Index 1

        // Apply pending configuration
        if (m_pendingSpectrumColor.isValid()) {
            m_miniPan->setSpectrumColor(m_pendingSpectrumColor);
        } else {
            // Default color based on VFO type
            m_miniPan->setSpectrumColor(QColor(m_type == VFO_A ? K4Colors::VfoAAmber : "#00FF00"));
        }
        if (m_pendingPassbandColor.isValid()) {
            m_miniPan->setPassbandColor(m_pendingPassbandColor);
        }
        if (!m_pendingMode.isEmpty()) {
            m_miniPan->setMode(m_pendingMode);
        }
        m_miniPan->setFilterBandwidth(m_pendingFilterBw);
        m_miniPan->setIfShift(m_pendingIfShift);
        m_miniPan->setCwPitch(m_pendingCwPitch);
        m_miniPan->setNotchFilter(m_pendingNotchEnabled, m_pendingNotchPitchHz);

        // Connect mini-pan click to show normal view and emit signal
        connect(m_miniPan, &MiniPanRhiWidget::clicked, this, [this]() {
            showNormal();
            emit miniPanClicked();
        });
    }
    m_stackedWidget->setCurrentIndex(1);
}

// Mini-pan configuration methods - store pending or apply immediately
void VFOWidget::setMiniPanMode(const QString &mode) {
    m_pendingMode = mode;
    if (m_miniPan)
        m_miniPan->setMode(mode);
}

void VFOWidget::setMiniPanFilterBandwidth(int bw) {
    m_pendingFilterBw = bw;
    if (m_miniPan)
        m_miniPan->setFilterBandwidth(bw);
}

void VFOWidget::setMiniPanIfShift(int shift) {
    m_pendingIfShift = shift;
    if (m_miniPan)
        m_miniPan->setIfShift(shift);
}

void VFOWidget::setMiniPanCwPitch(int pitch) {
    m_pendingCwPitch = pitch;
    if (m_miniPan)
        m_miniPan->setCwPitch(pitch);
}

void VFOWidget::setMiniPanNotchFilter(bool enabled, int pitchHz) {
    m_pendingNotchEnabled = enabled;
    m_pendingNotchPitchHz = pitchHz;
    if (m_miniPan)
        m_miniPan->setNotchFilter(enabled, pitchHz);
}

void VFOWidget::setMiniPanSpectrumColor(const QColor &color) {
    m_pendingSpectrumColor = color;
    if (m_miniPan)
        m_miniPan->setSpectrumColor(color);
}

void VFOWidget::setMiniPanPassbandColor(const QColor &color) {
    m_pendingPassbandColor = color;
    if (m_miniPan)
        m_miniPan->setPassbandColor(color);
}

void VFOWidget::showNormal() {
    m_stackedWidget->setCurrentIndex(0);
}

bool VFOWidget::isMiniPanVisible() const {
    return m_stackedWidget->currentIndex() == 1;
}
