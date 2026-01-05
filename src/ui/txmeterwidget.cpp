#include "txmeterwidget.h"
#include <QPainter>
#include <QLinearGradient>

namespace {
const QString DarkBackground = "#0d0d0d";
const QString TrackBackground = "#1a1a1a";
const QString LabelBoxBorder = "#8B0000"; // Dark red border for label boxes
const QString LabelBoxFill = "#1a1a1a";
const QString MeterBarColor = "#8B0000";     // Dark red/maroon like IC-7760
const QString MeterBarHighlight = "#CD5C5C"; // Lighter red for gradient
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
}

void TxMeterWidget::setPower(double watts, bool isQrp) {
    m_power = watts;
    m_isQrp = isQrp;
    update();
}

void TxMeterWidget::setAlc(int bars) {
    m_alc = qBound(0, bars, 10);
    update();
}

void TxMeterWidget::setCompression(int dB) {
    m_compression = qBound(0, dB, 30);
    update();
}

void TxMeterWidget::setSwr(double ratio) {
    m_swr = qMax(1.0, ratio);
    update();
}

void TxMeterWidget::setCurrent(double amps) {
    m_current = qMax(0.0, amps);
    update();
}

void TxMeterWidget::setTxMeters(int alc, int compDb, double fwdPower, double swr) {
    m_alc = qBound(0, alc, 10);
    m_compression = qBound(0, compDb, 30);
    m_power = fwdPower;
    m_swr = qMax(1.0, swr);
    update();
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

    // === Po (Power) ===
    {
        double maxPower = m_isQrp ? 10.0 : 100.0;
        double fillRatio = qMin(m_power / maxPower, 1.0);
        QStringList labels =
            m_isQrp ? QStringList{"0", "2", "4", "6", "8", "10W"} : QStringList{"0", "20", "40", "60", "80", "100W"};
        drawMeterRow(painter, y, rowHeight, "Po", fillRatio, labels, scaleFont, barStartX, barWidth, barHeight);
        y += rowHeight + spacing;
    }

    // === ALC ===
    {
        double fillRatio = m_alc / 10.0;
        QStringList labels = {"", "", "", "", "", "", "", "", "", "", ""}; // Just tick marks
        drawMeterRow(painter, y, rowHeight, "ALC", fillRatio, labels, scaleFont, barStartX, barWidth, barHeight);
        y += rowHeight + spacing;
    }

    // === COMP ===
    {
        double fillRatio = m_compression / 25.0; // Scale to 25 dB for visual
        QStringList labels = {"0", "5", "10", "15", "20", "dB"};
        drawMeterRow(painter, y, rowHeight, "COMP", fillRatio, labels, scaleFont, barStartX, barWidth, barHeight);
        y += rowHeight + spacing;
    }

    // === SWR ===
    {
        // SWR scale: 1.0 to 3.0 with infinity beyond
        // Map 1.0-3.0 to 0-1.0 fill ratio
        double fillRatio = qMin((m_swr - 1.0) / 2.0, 1.0);
        QStringList labels = {"1", "1.5", "2", "2.5", "3", QString::fromUtf8("\u221E")};
        drawMeterRow(painter, y, rowHeight, "SWR", fillRatio, labels, scaleFont, barStartX, barWidth, barHeight);
        y += rowHeight + spacing;
    }

    // === Id (PA Drain Current) ===
    {
        double fillRatio = qMin(m_current / 25.0, 1.0);
        QStringList labels = {"0", "5", "10", "15", "20", "25A"};
        drawMeterRow(painter, y, rowHeight, "Id", fillRatio, labels, scaleFont, barStartX, barWidth, barHeight);
    }
}

void TxMeterWidget::drawMeterRow(QPainter &painter, int y, int rowHeight, const QString &label, double fillRatio,
                                 const QStringList &scaleLabels, const QFont &scaleFont, int barStartX, int barWidth,
                                 int barHeight) {
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
        gradient.setColorAt(0.0, QColor(MeterBarColor));
        gradient.setColorAt(0.7, QColor(MeterBarColor));
        gradient.setColorAt(1.0, QColor(MeterBarHighlight));
        painter.fillRect(barStartX + 1, barY + 1, fillWidth - 2, barHeight - 2, gradient);
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
