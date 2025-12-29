#include "spectrum.h"
#include <QPainter>
#include <QPainterPath>

SpectrumWidget::SpectrumWidget(QWidget *parent)
    : QWidget(parent)
    , m_startFreq(0)
    , m_endFreq(48000)
{
    setMinimumHeight(150);
    setStyleSheet("background-color: black;");
}

void SpectrumWidget::setData(const QVector<float> &spectrum)
{
    m_spectrum = spectrum;
    update();
}

void SpectrumWidget::setFrequencyRange(double startHz, double endHz)
{
    m_startFreq = startHz;
    m_endFreq = endHz;
}

void SpectrumWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (m_spectrum.isEmpty()) return;

    painter.setPen(Qt::green);
    const int w = width();
    const int h = height();

    QPainterPath path;
    for (int i = 0; i < m_spectrum.size(); ++i) {
        float x = (float)i / m_spectrum.size() * w;
        float y = h - (m_spectrum[i] * h);
        if (i == 0) path.moveTo(x, y);
        else path.lineTo(x, y);
    }
    painter.drawPath(path);
}
