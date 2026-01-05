#include "smeterwidget.h"
#include <QPainter>
#include <QLinearGradient>

// K4 Color constants
namespace {
const QString DarkBackground = "#0d0d0d";
const QString TextGray = "#999999";
const QString InactiveGray = "#666666";
const QString VfoAAmber = "#FFB000";
} // namespace

SMeterWidget::SMeterWidget(QWidget *parent) : QWidget(parent), m_value(0.0), m_peakValue(0.0), m_color(VfoAAmber) {
    setMinimumHeight(16);
    setMaximumHeight(20);
    // Width adjusted to fit within VFO stacked widget (200px max)
    setMinimumWidth(180);
    setMaximumWidth(200);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // Setup peak decay timer
    m_peakDecayTimer = new QTimer(this);
    connect(m_peakDecayTimer, &QTimer::timeout, this, &SMeterWidget::decayPeak);
    m_peakDecayTimer->start(PeakDecayIntervalMs);
}

void SMeterWidget::setValue(double sValue) {
    m_value = sValue;
    // Update peak if new value is higher
    if (sValue > m_peakValue) {
        m_peakValue = sValue;
    }
    update();
}

void SMeterWidget::setColor(const QColor &color) {
    m_color = color;
    update();
}

void SMeterWidget::decayPeak() {
    if (m_peakValue > m_value) {
        m_peakValue -= PeakDecayRate;
        if (m_peakValue < m_value) {
            m_peakValue = m_value;
        }
        update();
    }
}

void SMeterWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // Background
    painter.fillRect(rect(), QColor(DarkBackground));

    // Draw scale labels (compact for 200px width)
    painter.setPen(QColor(TextGray));
    QFont scaleFont = font();
    scaleFont.setPointSize(6);
    painter.setFont(scaleFont);

    // Compact S-meter scale: fewer labels, shorter text
    QStringList labels = {"1", "3", "5", "7", "9", "20", "40", "60"};
    int labelWidth = w / labels.size();
    for (int i = 0; i < labels.size(); i++) {
        int x = i * labelWidth;
        painter.drawText(x, 0, labelWidth, h / 2, Qt::AlignCenter, labels[i]);
    }

    // Draw meter bar
    // S0-S9 = 0-9, S9+10 = 10, S9+20 = 11, etc.
    double maxValue = 15.0; // S9+60
    double fillRatio = qMin(m_value / maxValue, 1.0);
    int barY = h / 2 + 2;
    int barHeight = h / 2 - 4;
    int barWidth = static_cast<int>(w * fillRatio);

    // Gradient fill - transitions happen earlier for better visibility
    // Scale: S1=0.07, S3=0.2, S5=0.33, S7=0.47, S9=0.6, +20=0.73, +40=0.87, +60=1.0
    QLinearGradient gradient(0, 0, w, 0);
    gradient.setColorAt(0.0, QColor("#00CC00"));  // Green (S1-S2)
    gradient.setColorAt(0.13, QColor("#00FF00")); // Bright green (S2-S3)
    gradient.setColorAt(0.25, QColor("#CCFF00")); // Yellow-green (S3-S5)
    gradient.setColorAt(0.40, QColor("#FFFF00")); // Yellow (S5-S7)
    gradient.setColorAt(0.55, QColor("#FF9900")); // Orange (S7-S9)
    gradient.setColorAt(0.70, QColor("#FF6600")); // Dark orange (S9+10-+30)
    gradient.setColorAt(0.85, QColor("#FF3300")); // Red-orange (S9+40)
    gradient.setColorAt(1.0, QColor("#FF0000"));  // Red (S9+60)

    painter.fillRect(2, barY, barWidth - 4, barHeight, gradient);

    // Draw peak indicator
    if (m_peakValue > 0.01) {
        double peakRatio = qMin(m_peakValue / maxValue, 1.0);
        int peakX = static_cast<int>(w * peakRatio);
        // Draw a small vertical line at peak position
        painter.setPen(QPen(QColor("#FFFFFF"), 2));
        painter.drawLine(peakX - 1, barY, peakX - 1, barY + barHeight);
    }

    // Border
    painter.setPen(QColor(InactiveGray));
    painter.drawRect(1, barY - 1, w - 3, barHeight + 1);
}
