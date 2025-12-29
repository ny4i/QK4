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

SMeterWidget::SMeterWidget(QWidget *parent) : QWidget(parent), m_value(0.0), m_color(VfoAAmber) {
    setMinimumHeight(16);
    setMaximumHeight(20);
    // Compressed width to leave room for filter indicator widget
    setMinimumWidth(280);
    setMaximumWidth(380);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void SMeterWidget::setValue(double sValue) {
    m_value = sValue;
    update();
}

void SMeterWidget::setColor(const QColor &color) {
    m_color = color;
    update();
}

void SMeterWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // Background
    painter.fillRect(rect(), QColor(DarkBackground));

    // Draw scale labels
    painter.setPen(QColor(TextGray));
    QFont scaleFont = font();
    scaleFont.setPointSize(7);
    painter.setFont(scaleFont);

    // S-meter scale: S1-S9 then +20, +40, +60
    QStringList labels = {"S1", "3", "5", "7", "9", "+20", "+40", "+60"};
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

    // Gradient fill - blue to green to yellow to red
    QLinearGradient gradient(0, 0, w, 0);
    gradient.setColorAt(0.0, QColor("#0066FF")); // Blue (S1-S3)
    gradient.setColorAt(0.4, QColor("#00FF00")); // Green (S5-S7)
    gradient.setColorAt(0.6, QColor("#FFFF00")); // Yellow (S9)
    gradient.setColorAt(0.8, QColor("#FF6600")); // Orange (S9+20)
    gradient.setColorAt(1.0, QColor("#FF0000")); // Red (S9+60)

    painter.fillRect(2, barY, barWidth - 4, barHeight, gradient);

    // Border
    painter.setPen(QColor(InactiveGray));
    painter.drawRect(1, barY - 1, w - 3, barHeight + 1);
}
