#include "txmeterwidget.h"
#include <QPainter>
#include <QLinearGradient>

namespace {
const QString DarkBackground = "#0d0d0d";
const QString TrackBackground = "#1a1a1a";
const QString LabelBoxBorder = "#666666";
const QString LabelBoxFill = "#1a1a1a";
const QString MeterBarColor = "#8B0000";     // Dark red/maroon for Id meter
const QString MeterBarHighlight = "#CD5C5C"; // Lighter red for Id gradient
const QString TextWhite = "#FFFFFF";
const QString TextGray = "#888888";
const QString ScaleGray = "#666666";
} // namespace

TxMeterWidget::TxMeterWidget(QWidget *parent) : QWidget(parent) {
    // 5 meters, each ~18px high with spacing
    setFixedHeight(95);
    setMinimumWidth(200);
    setMaximumWidth(380);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // Enable custom painting
    setAttribute(Qt::WA_OpaquePaintEvent);

    // Setup decay timer
    m_decayTimer = new QTimer(this);
    connect(m_decayTimer, &QTimer::timeout, this, &TxMeterWidget::decayValues);
    m_decayTimer->start(DecayIntervalMs);
}

void TxMeterWidget::setPower(double watts, bool isQrp) {
    m_isQrp = isQrp;
    double maxPower = isQrp ? 10.0 : 100.0;
    double ratio = qMin(watts / maxPower, 1.0);
    m_powerTarget = ratio;
    if (ratio > m_powerDisplay) {
        m_powerDisplay = ratio; // Instant rise
    }
    if (ratio > m_powerPeak) {
        m_powerPeak = ratio;
    }
    update();
}

void TxMeterWidget::setAlc(int bars) {
    double ratio = qBound(0, bars, 10) / 10.0;
    m_alcTarget = ratio;
    if (ratio > m_alcDisplay) {
        m_alcDisplay = ratio;
    }
    if (ratio > m_alcPeak) {
        m_alcPeak = ratio;
    }
    update();
}

void TxMeterWidget::setCompression(int dB) {
    double ratio = qBound(0, dB, 25) / 25.0; // Scale to 25 dB for visual
    m_compTarget = ratio;
    if (ratio > m_compDisplay) {
        m_compDisplay = ratio;
    }
    if (ratio > m_compPeak) {
        m_compPeak = ratio;
    }
    update();
}

void TxMeterWidget::setSwr(double ratio) {
    // SWR scale: 1.0 to 3.0, map to 0-1 fill ratio
    double fillRatio = qMin((qMax(1.0, ratio) - 1.0) / 2.0, 1.0);
    m_swrTarget = fillRatio;
    if (fillRatio > m_swrDisplay) {
        m_swrDisplay = fillRatio;
    }
    if (fillRatio > m_swrPeak) {
        m_swrPeak = fillRatio;
    }
    update();
}

void TxMeterWidget::setCurrent(double amps) {
    double ratio = qMin(qMax(0.0, amps) / 25.0, 1.0);
    m_currentTarget = ratio;
    if (ratio > m_currentDisplay) {
        m_currentDisplay = ratio;
    }
    if (ratio > m_currentPeak) {
        m_currentPeak = ratio;
    }
    update();
}

void TxMeterWidget::setTxMeters(int alc, int compDb, double fwdPower, double swr) {
    // Power - use appropriate max based on mode
    double maxPower;
    if (m_kpa1500Enabled) {
        maxPower = 1900.0;
    } else if (m_isQrp) {
        maxPower = 10.0;
    } else {
        maxPower = 110.0; // K4 goes to 110W
    }
    double powerRatio = qMin(fwdPower / maxPower, 1.0);
    m_powerTarget = powerRatio;
    if (powerRatio > m_powerDisplay)
        m_powerDisplay = powerRatio;
    if (powerRatio > m_powerPeak)
        m_powerPeak = powerRatio;

    // ALC
    double alcRatio = qBound(0, alc, 10) / 10.0;
    m_alcTarget = alcRatio;
    if (alcRatio > m_alcDisplay)
        m_alcDisplay = alcRatio;
    if (alcRatio > m_alcPeak)
        m_alcPeak = alcRatio;

    // Compression
    double compRatio = qBound(0, compDb, 25) / 25.0;
    m_compTarget = compRatio;
    if (compRatio > m_compDisplay)
        m_compDisplay = compRatio;
    if (compRatio > m_compPeak)
        m_compPeak = compRatio;

    // SWR
    double swrRatio = qMin((qMax(1.0, swr) - 1.0) / 2.0, 1.0);
    m_swrTarget = swrRatio;
    if (swrRatio > m_swrDisplay)
        m_swrDisplay = swrRatio;
    if (swrRatio > m_swrPeak)
        m_swrPeak = swrRatio;

    update();
}

void TxMeterWidget::setSMeter(double sValue) {
    // S-meter value: 0-9 for S1-S9, 9+ for dB over S9 (S9+10 = 10, S9+20 = 11, etc.)
    double maxValue = 15.0; // S9+60 (S9 = 9, +60dB/10 = 6 more = 15)
    double ratio = qMin(sValue / maxValue, 1.0);
    m_sMeterTarget = ratio;
    if (ratio > m_sMeterDisplay) {
        m_sMeterDisplay = ratio; // Instant rise
    }
    if (ratio > m_sMeterPeak) {
        m_sMeterPeak = ratio;
    }
    update();
}

void TxMeterWidget::setTransmitting(bool isTx) {
    if (m_isTransmitting != isTx) {
        m_isTransmitting = isTx;
        update();
    }
}

void TxMeterWidget::setAmplifierEnabled(bool enabled) {
    if (m_kpa1500Enabled != enabled) {
        m_kpa1500Enabled = enabled;
        update();
    }
}

void TxMeterWidget::decayValues() {
    bool needsUpdate = false;

    // Decay display values toward targets
    auto decayValue = [&](double &display, double target) {
        if (display > target) {
            display -= DecayRate;
            if (display < target)
                display = target;
            needsUpdate = true;
        }
    };

    // Decay peak values (slower)
    auto decayPeak = [&](double &peak, double display) {
        if (peak > display) {
            peak -= PeakDecayRate;
            if (peak < display)
                peak = display;
            needsUpdate = true;
        }
    };

    decayValue(m_powerDisplay, m_powerTarget);
    decayValue(m_alcDisplay, m_alcTarget);
    decayValue(m_compDisplay, m_compTarget);
    decayValue(m_swrDisplay, m_swrTarget);
    decayValue(m_currentDisplay, m_currentTarget);
    decayValue(m_sMeterDisplay, m_sMeterTarget); // S-meter decay

    decayPeak(m_powerPeak, m_powerDisplay);
    decayPeak(m_alcPeak, m_alcDisplay);
    decayPeak(m_compPeak, m_compDisplay);
    decayPeak(m_swrPeak, m_swrDisplay);
    decayPeak(m_currentPeak, m_currentDisplay);
    decayPeak(m_sMeterPeak, m_sMeterDisplay); // S-meter peak decay

    if (needsUpdate) {
        update();
    }
}

void TxMeterWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false); // Crisp pixel lines

    int w = width();
    int h = height();

    // Background
    painter.fillRect(rect(), QColor(DarkBackground));

    // Layout constants
    const int labelWidth = 36; // Width of label box (Po, ALC, etc.)
    const int rowHeight = 17;  // Height per meter row
    const int barStartX = labelWidth + 4;
    const int barWidth = w - barStartX - 4;
    const int barHeight = 8;
    const int spacing = 2;

    // Font for labels
    QFont labelFont = font();
    labelFont.setPointSize(8);
    labelFont.setBold(true);
    painter.setFont(labelFont);

    // Scale font (smaller)
    QFont scaleFont = font();
    scaleFont.setPointSize(6);

    int y = 0;

    // === S/Po (S-Meter when RX, Power when TX) - Gradient ===
    {
        QStringList labels;
        double displayValue, peakValue;
        MeterType meterType = MeterType::Gradient;

        if (!m_isTransmitting) {
            // RX mode: show S-meter
            labels = {"1", "3", "5", "7", "9", "+20", "+40", "+60"};
            displayValue = m_sMeterDisplay;
            peakValue = m_sMeterPeak;
        } else if (m_kpa1500Enabled) {
            // TX mode with KPA1500: 0-1900W scale
            labels = {"0", "380", "760", "1140", "1500", "1900W"};
            displayValue = m_powerDisplay;
            peakValue = m_powerPeak;
            meterType = MeterType::KPA1500;
        } else if (m_isQrp) {
            // TX mode with K4 QRP: 0-10W scale
            labels = {"0", "2", "4", "6", "8", "10W"};
            displayValue = m_powerDisplay;
            peakValue = m_powerPeak;
        } else {
            // TX mode with K4: 0-110W scale
            labels = {"0", "22", "44", "66", "88", "110W"};
            displayValue = m_powerDisplay;
            peakValue = m_powerPeak;
        }

        drawMeterRow(painter, y, rowHeight, "S/Po", displayValue, peakValue, labels, scaleFont, barStartX, barWidth,
                     barHeight, meterType);
        y += rowHeight + spacing;
    }

    // === ALC - Gradient ===
    {
        QStringList labels = {"", "", "", "", "", "", "", "", "", "", ""}; // Just tick marks
        drawMeterRow(painter, y, rowHeight, "ALC", m_alcDisplay, m_alcPeak, labels, scaleFont, barStartX, barWidth,
                     barHeight, MeterType::Gradient);
        y += rowHeight + spacing;
    }

    // === COMP - Gradient ===
    {
        QStringList labels = {"0", "5", "10", "15", "20", "dB"};
        drawMeterRow(painter, y, rowHeight, "COMP", m_compDisplay, m_compPeak, labels, scaleFont, barStartX, barWidth,
                     barHeight, MeterType::Gradient);
        y += rowHeight + spacing;
    }

    // === SWR - Gradient ===
    {
        QStringList labels = {"1", "1.5", "2", "2.5", "3", QString::fromUtf8("\u221E")};
        drawMeterRow(painter, y, rowHeight, "SWR", m_swrDisplay, m_swrPeak, labels, scaleFont, barStartX, barWidth,
                     barHeight, MeterType::Gradient);
        y += rowHeight + spacing;
    }

    // === Id (PA Drain Current) - Red ===
    {
        QStringList labels = {"0", "5", "10", "15", "20", "25A"};
        drawMeterRow(painter, y, rowHeight, "Id", m_currentDisplay, m_currentPeak, labels, scaleFont, barStartX,
                     barWidth, barHeight, MeterType::Red);
    }
}

void TxMeterWidget::drawMeterRow(QPainter &painter, int y, int rowHeight, const QString &label, double fillRatio,
                                 double peakRatio, const QStringList &scaleLabels, const QFont &scaleFont,
                                 int barStartX, int barWidth, int barHeight, MeterType type) {
    // Label box on the left
    QRect labelRect(2, y + 2, 32, rowHeight - 4);
    painter.setPen(QColor(LabelBoxBorder));
    painter.setBrush(QColor(LabelBoxFill));
    painter.drawRect(labelRect);

    // Label text
    painter.setPen(QColor(TextWhite));
    QFont labelFont = font();
    labelFont.setPointSize(8);
    labelFont.setBold(true);
    painter.setFont(labelFont);
    painter.drawText(labelRect, Qt::AlignCenter, label);

    // Meter bar track (dark background)
    int barY = y + 2;
    QRect trackRect(barStartX, barY, barWidth, barHeight);
    painter.fillRect(trackRect, QColor(TrackBackground));
    painter.setPen(QColor(ScaleGray));
    painter.drawRect(trackRect);

    // Filled meter bar
    if (fillRatio > 0.001) {
        int fillWidth = static_cast<int>(barWidth * fillRatio);
        QLinearGradient gradient(barStartX, 0, barStartX + barWidth, 0);

        if (type == MeterType::Gradient) {
            // S-meter style gradient: green → yellow → orange → red
            gradient.setColorAt(0.0, QColor("#00CC00"));  // Green
            gradient.setColorAt(0.13, QColor("#00FF00")); // Bright green
            gradient.setColorAt(0.25, QColor("#CCFF00")); // Yellow-green
            gradient.setColorAt(0.40, QColor("#FFFF00")); // Yellow
            gradient.setColorAt(0.55, QColor("#FF9900")); // Orange
            gradient.setColorAt(0.70, QColor("#FF6600")); // Dark orange
            gradient.setColorAt(0.85, QColor("#FF3300")); // Red-orange
            gradient.setColorAt(1.0, QColor("#FF0000"));  // Red
        } else if (type == MeterType::KPA1500) {
            // KPA1500 power gradient: green zone to 1500W, yellow-red for 1500-1900W
            // 1500W = 79% of 1900W, 1700W = 89% of 1900W
            gradient.setColorAt(0.0, QColor("#00CC00"));  // Green (0W)
            gradient.setColorAt(0.50, QColor("#00FF00")); // Bright green (~950W)
            gradient.setColorAt(0.79, QColor("#CCFF00")); // Yellow-green (1500W - safe max)
            gradient.setColorAt(0.89, QColor("#FF9900")); // Orange (1700W)
            gradient.setColorAt(1.0, QColor("#FF0000"));  // Red (1900W - absolute max)
        } else {
            // Red style for Id meter
            gradient.setColorAt(0.0, QColor(MeterBarColor));
            gradient.setColorAt(0.7, QColor(MeterBarColor));
            gradient.setColorAt(1.0, QColor(MeterBarHighlight));
        }
        painter.fillRect(barStartX + 1, barY + 1, fillWidth - 2, barHeight - 2, gradient);
    }

    // Draw peak indicator
    if (peakRatio > 0.01) {
        int peakX = barStartX + static_cast<int>(barWidth * peakRatio);
        painter.setPen(QPen(QColor("#FFFFFF"), 2));
        painter.drawLine(peakX - 1, barY, peakX - 1, barY + barHeight);
    }

    // Scale labels below bar
    painter.setFont(scaleFont);
    painter.setPen(QColor(TextGray));
    int scaleY = barY + barHeight + 1;
    int numLabels = scaleLabels.size();
    if (numLabels > 0) {
        for (int i = 0; i < numLabels; i++) {
            if (scaleLabels[i].isEmpty())
                continue;
            int x = barStartX + (barWidth * i) / (numLabels - 1);
            // Center the label, but keep last one right-aligned
            int labelW = 20;
            int labelX = (i == numLabels - 1) ? x - labelW : x - labelW / 2;
            painter.drawText(labelX, scaleY, labelW, 8, Qt::AlignCenter, scaleLabels[i]);
        }
    }

    // Draw tick marks on the bar
    painter.setPen(QColor(ScaleGray));
    for (int i = 0; i < numLabels; i++) {
        int x = barStartX + (barWidth * i) / (numLabels - 1);
        painter.drawLine(x, barY, x, barY + 2);
    }
}
