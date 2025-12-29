#include "waterfall.h"
#include <QPainter>

WaterfallWidget::WaterfallWidget(QWidget *parent)
    : QWidget(parent)
    , m_currentLine(0)
{
    setMinimumHeight(200);
    setStyleSheet("background-color: black;");
}

void WaterfallWidget::addLine(const QVector<float> &spectrum)
{
    if (m_image.isNull() || m_image.width() != spectrum.size()) {
        m_image = QImage(spectrum.size(), height(), QImage::Format_RGB32);
        m_image.fill(Qt::black);
        m_currentLine = 0;
    }

    // Scroll image up
    if (m_currentLine >= m_image.height()) {
        for (int y = 0; y < m_image.height() - 1; ++y) {
            memcpy(m_image.scanLine(y), m_image.scanLine(y + 1), 
                   m_image.bytesPerLine());
        }
        m_currentLine = m_image.height() - 1;
    }

    // Draw new line
    for (int x = 0; x < spectrum.size(); ++x) {
        m_image.setPixel(x, m_currentLine, valueToColor(spectrum[x]));
    }
    m_currentLine++;
    update();
}

void WaterfallWidget::clear()
{
    m_image.fill(Qt::black);
    m_currentLine = 0;
    update();
}

void WaterfallWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    if (!m_image.isNull()) {
        painter.drawImage(rect(), m_image);
    } else {
        painter.fillRect(rect(), Qt::black);
    }
}

void WaterfallWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    m_image = QImage(width(), height(), QImage::Format_RGB32);
    m_image.fill(Qt::black);
    m_currentLine = 0;
}

QRgb WaterfallWidget::valueToColor(float value)
{
    value = qBound(0.0f, value, 1.0f);
    int r = qBound(0, (int)(value * 255 * 2), 255);
    int g = qBound(0, (int)((value - 0.25f) * 255 * 2), 255);
    int b = qBound(0, (int)((0.5f - value) * 255 * 2), 255);
    return qRgb(r, g, b);
}
