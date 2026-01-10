#include "vfowidget.h"
#include "k4styles.h"
#include "txmeterwidget.h"
#include "../dsp/minipan_rhi.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QFontMetrics>

VFOWidget::VFOWidget(VFOType type, QWidget *parent)
    : QWidget(parent), m_type(type),
      m_primaryColor(type == VFO_A ? K4Styles::Colors::VfoAAmber : K4Styles::Colors::VfoBCyan),
      m_inactiveColor(K4Styles::Colors::InactiveGray) {
    setupUi();
}

void VFOWidget::setupUi() {
    setStyleSheet(QString("background-color: %1;").arg(K4Styles::Colors::Background));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 4, 8, 4);
    mainLayout->setSpacing(2);

    // Row 1: Frequency (aligned with S-meter)
    auto *freqRow = new QHBoxLayout();
    m_frequencyLabel = new QLabel("---.---.---", this);
    m_frequencyLabel->setStyleSheet(
        QString(
            "color: %1; font-size: 32px; font-weight: bold; font-family: 'JetBrains Mono', 'Courier New', monospace;")
            .arg(K4Styles::Colors::TextWhite));
    m_frequencyLabel->setFixedHeight(40);
    m_frequencyLabel->setCursor(Qt::PointingHandCursor);
    m_frequencyLabel->installEventFilter(this);

    // Frequency entry line edit (hidden by default)
    m_frequencyEdit = new QLineEdit(this);
    m_frequencyEdit->setStyleSheet(
        QString("QLineEdit { color: %1; background-color: %2; border: 2px solid %3; "
                "font-size: 32px; font-weight: bold; font-family: 'JetBrains Mono', 'Courier New', monospace; "
                "padding: 2px 4px; }")
            .arg(K4Styles::Colors::TextWhite)
            .arg(K4Styles::Colors::DarkBackground)
            .arg(m_primaryColor));
    m_frequencyEdit->setFixedHeight(40);
    m_frequencyEdit->setMaxLength(11); // Max 11 digits per K4 spec
    m_frequencyEdit->setAlignment(Qt::AlignRight);
    m_frequencyEdit->hide();
    m_frequencyEdit->installEventFilter(this);
    connect(m_frequencyEdit, &QLineEdit::returnPressed, this, &VFOWidget::onFrequencyEditFinished);

    if (m_type == VFO_A) {
        // VFO A: frequency left-aligned with S-meter
        freqRow->addWidget(m_frequencyLabel);
        freqRow->addWidget(m_frequencyEdit);
        freqRow->addStretch();
    } else {
        // VFO B: frequency right-aligned with S-meter
        freqRow->addStretch();
        freqRow->addWidget(m_frequencyEdit);
        freqRow->addWidget(m_frequencyLabel);
    }
    mainLayout->addLayout(freqRow);

    // Stacked widget for normal content vs mini-pan
    // Use dynamic height - stacked widget resizes based on active page
    // Use Maximum horizontal policy so it doesn't expand beyond content width
    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    m_stackedWidget->setMaximumWidth(210); // Fit APF indicator while keeping symmetry

    // Page 0: Normal content (multifunction meter + features)
    // Height must match MiniPanRhiWidget (110px) to prevent layout shift when toggling
    m_normalContent = new QWidget(m_stackedWidget);
    m_normalContent->setFixedSize(210, 110); // Fit APF indicator while keeping symmetry
    auto *normalLayout = new QVBoxLayout(m_normalContent);
    normalLayout->setContentsMargins(0, 0, 0, 0);
    normalLayout->setSpacing(2);

    // Row 2: Multifunction Meter (S/Po, ALC, COMP, SWR, Id) - replaces old S-meter
    // Meter fills full width of normal content (both are 200px)
    m_txMeter = new TxMeterWidget(m_normalContent);
    m_txMeter->setFixedWidth(200); // Match mini-pan width
    normalLayout->addWidget(m_txMeter);

    // Row 3: AGC, PRE, ATT, NB, NR labels (aligned with meter)
    auto *featuresContainer = new QWidget(m_normalContent);
    auto *featuresRow = new QHBoxLayout(featuresContainer);
    featuresRow->setContentsMargins(0, 0, 0, 0);
    featuresRow->setSpacing(3); // Tighter spacing to fit APF indicator

    m_agcLabel = new QLabel("AGC-S", featuresContainer);
    m_agcLabel->setStyleSheet("color: #999999; font-size: 11px;");

    m_preampLabel = new QLabel("PRE", featuresContainer);
    m_preampLabel->setStyleSheet("color: #999999; font-size: 11px;");

    m_attLabel = new QLabel("ATT", featuresContainer);
    m_attLabel->setStyleSheet("color: #999999; font-size: 11px;");

    m_nbLabel = new QLabel("NB", featuresContainer);
    m_nbLabel->setStyleSheet("color: #999999; font-size: 11px;");

    m_nrLabel = new QLabel("NR", featuresContainer);
    m_nrLabel->setStyleSheet("color: #999999; font-size: 11px;");

    m_ntchLabel = new QLabel("NTCH", featuresContainer);
    m_ntchLabel->setStyleSheet("color: #999999; font-size: 11px;");

    m_apfLabel = new QLabel("APF", featuresContainer);
    m_apfLabel->setMinimumWidth(42); // Wide enough for "APF-150"
    m_apfLabel->setStyleSheet("color: #999999; font-size: 11px;");

    // Add labels to layout
    featuresRow->addWidget(m_agcLabel);
    featuresRow->addWidget(m_preampLabel);
    featuresRow->addWidget(m_attLabel);
    featuresRow->addWidget(m_nbLabel);
    featuresRow->addWidget(m_nrLabel);
    featuresRow->addWidget(m_ntchLabel);
    featuresRow->addWidget(m_apfLabel);

    // Features row is left-aligned within its container for both VFOs
    // VFO B's entire container is pushed right by stackedRow layout
    normalLayout->addWidget(featuresContainer, 0, Qt::AlignLeft);

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

    // NOTE: TX Meter (m_txMeter) is now created in normal content area above
    // It displays multifunction meters: S/Po, ALC, COMP, SWR, Id

    // Install event filter for click-to-toggle on normal content
    m_normalContent->installEventFilter(this);
}

bool VFOWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_normalContent && event->type() == QEvent::MouseButtonPress) {
        emit normalContentClicked();
        return true;
    }

    // Click on frequency label to start editing
    if (watched == m_frequencyLabel && event->type() == QEvent::MouseButtonPress) {
        startFrequencyEntry();
        return true;
    }

    // Handle key events in frequency edit
    if (watched == m_frequencyEdit && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            cancelFrequencyEntry();
            return true;
        }
    }

    // Handle focus loss on frequency edit
    if (watched == m_frequencyEdit && event->type() == QEvent::FocusOut) {
        cancelFrequencyEntry();
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

void VFOWidget::setFrequency(const QString &formatted) {
    m_frequencyLabel->setText(formatted);
    update(); // Repaint to update tuning rate indicator position
}

void VFOWidget::setTuningRate(int rate) {
    if (rate >= 0 && rate <= 5 && rate != m_tuningRate) {
        m_tuningRate = rate;
        update(); // Repaint to show new indicator position
    }
}

void VFOWidget::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);

    // Draw tuning rate indicator after base paint
    QPainter painter(this);
    drawTuningRateIndicator(painter);
}

void VFOWidget::drawTuningRateIndicator(QPainter &painter) {
    QString freqText = m_frequencyLabel->text();
    if (freqText.isEmpty() || freqText.startsWith("-"))
        return; // No frequency set yet

    // Get frequency label geometry in widget coordinates
    QPoint labelPos = m_frequencyLabel->mapTo(this, QPoint(0, 0));
    QRect labelRect(labelPos, m_frequencyLabel->size());

    // Get font metrics for the frequency label's font
    QFont font = m_frequencyLabel->font();
    QFontMetrics fm(font);

    // VT rate maps to digit position from right:
    // Rate 0 = 1Hz (pos 0), Rate 1 = 10Hz (pos 1), Rate 2 = 100Hz (pos 2)
    // Rate 3 = 1kHz (pos 3), Rate 4 = 10kHz (pos 4)
    // Rate 5 = 100Hz (pos 2) - special case: KHZ button tunes at 100Hz
    int digitPosition = (m_tuningRate == 5) ? 2 : m_tuningRate;

    // Find the character index for the digit at digitPosition from right
    // Frequency format: "7.204.000" or "14.225.350"
    // We need to skip decimal separators when counting digits from right
    int digitCount = 0;
    int charIndex = -1;
    for (int i = freqText.length() - 1; i >= 0 && charIndex < 0; --i) {
        if (freqText[i].isDigit()) {
            if (digitCount == digitPosition) {
                charIndex = i;
                break;
            }
            digitCount++;
        }
    }

    if (charIndex < 0)
        return; // Digit position not found

    // Calculate X position of the target character
    // Using monospace font, so each character has same width
    int charWidth = fm.horizontalAdvance('0');

    // Calculate position from left edge of label
    // For monospace, character at index i starts at i * charWidth
    int charX = 0;
    for (int i = 0; i < charIndex; ++i) {
        charX += fm.horizontalAdvance(freqText[i]);
    }

    // Underline properties
    const int spacing = 2;   // pixels below digit baseline
    const int thickness = 2; // underline height

    // Calculate underline position in widget coordinates
    int underlineX = labelRect.left() + charX;
    int underlineY = labelRect.bottom() + spacing;
    int underlineWidth = charWidth;

    // Draw underline in frequency text color (white)
    painter.fillRect(underlineX, underlineY, underlineWidth, thickness, QColor("#FFFFFF"));
}

void VFOWidget::setSMeterValue(double value) {
    if (m_txMeter)
        m_txMeter->setSMeter(value);
}

void VFOWidget::setAGC(const QString &mode) {
    m_agcLabel->setText(mode);
    // AGC is always shown, color indicates active state
    bool active = !mode.contains("-") || mode == "AGC-F" || mode == "AGC-S" || mode == "AGC-M";
    m_agcLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(active ? "#FFFFFF" : "#999999"));
}

void VFOWidget::setPreamp(bool on, int level) {
    // Show level when active (PRE-1, PRE-2, PRE-3), just "PRE" when off
    if (on && level > 0) {
        m_preampLabel->setText(QString("PRE-%1").arg(level));
    } else {
        m_preampLabel->setText("PRE");
    }
    m_preampLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(on ? "#FFFFFF" : "#999999"));
}

void VFOWidget::setAtt(bool on, int level) {
    // Show level when active (ATT-3, ATT-6, etc.), just "ATT" when off
    if (on && level > 0) {
        m_attLabel->setText(QString("ATT-%1").arg(level));
    } else {
        m_attLabel->setText("ATT");
    }
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

void VFOWidget::setApf(bool enabled, int bandwidth) {
    QString text = "APF";
    if (enabled) {
        static const char *bwNames[] = {"30", "50", "150"};
        text = QString("APF-%1").arg(bwNames[qBound(0, bandwidth, 2)]);
    }
    m_apfLabel->setText(text);
    m_apfLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(enabled ? "#FFFFFF" : "#999999"));
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
            m_miniPan->setSpectrumColor(QColor(m_type == VFO_A ? K4Styles::Colors::VfoAAmber : "#00FF00"));
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

// Multifunction meter methods (S/Po, ALC, COMP, SWR, Id)
void VFOWidget::setTransmitting(bool isTx) {
    if (m_txMeter)
        m_txMeter->setTransmitting(isTx);
}

void VFOWidget::setTxMeters(int alc, int compDb, double fwdPower, double swr) {
    if (m_txMeter)
        m_txMeter->setTxMeters(alc, compDb, fwdPower, swr);
}

void VFOWidget::setTxMeterCurrent(double amps) {
    if (m_txMeter)
        m_txMeter->setCurrent(amps);
}

void VFOWidget::startFrequencyEntry() {
    if (!m_frequencyEdit)
        return;

    // Get current frequency text, remove separators (dots) for editing
    QString currentFreq = m_frequencyLabel->text();
    currentFreq.remove('.');
    currentFreq.remove('-'); // Remove placeholders if no frequency set

    // Show edit field, hide label
    m_frequencyLabel->hide();
    m_frequencyEdit->setText(currentFreq);
    m_frequencyEdit->show();
    m_frequencyEdit->setFocus();
    m_frequencyEdit->selectAll();
}

void VFOWidget::onFrequencyEditFinished() {
    if (!m_frequencyEdit)
        return;

    QString enteredFreq = m_frequencyEdit->text().trimmed();

    // Validate: only digits allowed
    bool valid = true;
    for (const QChar &c : enteredFreq) {
        if (!c.isDigit()) {
            valid = false;
            break;
        }
    }

    // Hide edit, show label
    m_frequencyEdit->hide();
    m_frequencyLabel->show();

    // Emit signal if valid and not empty
    if (valid && !enteredFreq.isEmpty()) {
        emit frequencyEntered(enteredFreq);
    }
}

void VFOWidget::cancelFrequencyEntry() {
    if (!m_frequencyEdit)
        return;

    // Hide edit, show label (no change to frequency)
    m_frequencyEdit->hide();
    m_frequencyLabel->show();
}
