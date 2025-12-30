#include "minipanwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QDebug>
#include <cmath>

// K4-style colors for consistency
namespace MiniPanColors {
const QColor DarkBackground("#0a0a0a");
const QColor SpectrumLine("#FFB000"); // Amber to match VFO A
const QColor GridLine("#333333");
} // namespace MiniPanColors

MiniPanWidget::MiniPanWidget(QWidget *parent) : QWidget(parent) {
    setFixedHeight(110); // Taller for more waterfall history (was 90, clipped to 60)
    setMinimumWidth(180);
    setMaximumWidth(200);

    // Initialize waterfall history buffer
    m_waterfallHistory.resize(WATERFALL_HISTORY);

    initColorLUT();
}

void MiniPanWidget::initColorLUT() {
    // Create 256-entry color lookup table (black -> blue -> cyan -> yellow -> red -> white)
    m_colorLUT.resize(256);

    for (int i = 0; i < 256; ++i) {
        float t = i / 255.0f;
        int r, g, b;

        if (t < 0.2f) {
            // Black to dark blue
            float s = t / 0.2f;
            r = 0;
            g = 0;
            b = static_cast<int>(60 * s);
        } else if (t < 0.4f) {
            // Dark blue to cyan
            float s = (t - 0.2f) / 0.2f;
            r = 0;
            g = static_cast<int>(180 * s);
            b = 60 + static_cast<int>(140 * s);
        } else if (t < 0.6f) {
            // Cyan to yellow
            float s = (t - 0.4f) / 0.2f;
            r = static_cast<int>(255 * s);
            g = 180 + static_cast<int>(75 * s);
            b = 200 - static_cast<int>(200 * s);
        } else if (t < 0.8f) {
            // Yellow to orange/red
            float s = (t - 0.6f) / 0.2f;
            r = 255;
            g = 255 - static_cast<int>(155 * s);
            b = 0;
        } else {
            // Orange/red to white
            float s = (t - 0.8f) / 0.2f;
            r = 255;
            g = 100 + static_cast<int>(155 * s);
            b = static_cast<int>(255 * s);
        }

        m_colorLUT[i] = qRgb(r, g, b);
    }
}

void MiniPanWidget::updateSpectrum(const QByteArray &bins) {
    if (bins.isEmpty())
        return;

    // Decompress MiniPAN data: byte / 10 (different from main panadapter)
    // Observed byte range: 0-24+ for noise to strong signals
    // After /10: 0.0 to 2.5+ dB relative range
    m_spectrum.resize(bins.size());
    for (int i = 0; i < bins.size(); ++i) {
        m_spectrum[i] = static_cast<quint8>(bins[i]) / 10.0f;
    }

    // Skip bins at edges to remove blank areas (~7.5% each side of ~1033 bins)
    const int SKIP_START = 75;
    const int SKIP_END = 75;

    if (m_spectrum.size() > SKIP_START + SKIP_END + 10) {
        int validSize = m_spectrum.size() - SKIP_START - SKIP_END;
        QVector<float> trimmed(validSize);
        for (int i = 0; i < validSize; ++i) {
            trimmed[i] = m_spectrum[SKIP_START + i];
        }
        m_spectrum = trimmed;
    }

    // Apply smoothing
    if (m_smoothedSpectrum.size() != m_spectrum.size()) {
        m_smoothedSpectrum = m_spectrum;
    } else {
        for (int i = 0; i < m_spectrum.size(); ++i) {
            m_smoothedSpectrum[i] =
                m_smoothingAlpha * m_spectrum[i] + (1.0f - m_smoothingAlpha) * m_smoothedSpectrum[i];
        }
    }

    // Update waterfall
    m_waterfallHistory[m_waterfallWriteIndex] = m_smoothedSpectrum;
    m_waterfallWriteIndex = (m_waterfallWriteIndex + 1) % WATERFALL_HISTORY;

    update();
}

void MiniPanWidget::clear() {
    m_spectrum.clear();
    m_smoothedSpectrum.clear();
    m_waterfallWriteIndex = 0;
    m_smoothedBaseline = 0.0f;
    for (int i = 0; i < m_waterfallHistory.size(); ++i) {
        m_waterfallHistory[i].clear();
    }
    update();
}

void MiniPanWidget::setSpectrumColor(const QColor &color) {
    if (m_spectrumColor != color) {
        m_spectrumColor = color;
        update();
    }
}

void MiniPanWidget::setPassbandColor(const QColor &color) {
    if (m_passbandColor != color) {
        m_passbandColor = color;
        update();
    }
}

void MiniPanWidget::setNotchFilter(bool enabled, int pitchHz) {
    if (m_notchEnabled != enabled || m_notchPitchHz != pitchHz) {
        m_notchEnabled = enabled;
        m_notchPitchHz = pitchHz;
        update();
    }
}

void MiniPanWidget::setMode(const QString &mode) {
    if (m_mode != mode) {
        m_mode = mode;
        m_bandwidthHz = bandwidthForMode(mode);
        update();
    }
}

void MiniPanWidget::setFilterBandwidth(int bwHz) {
    if (m_filterBw != bwHz) {
        m_filterBw = bwHz;
        update();
    }
}

void MiniPanWidget::setIfShift(int shift) {
    if (m_ifShift != shift) {
        m_ifShift = shift;
        update();
    }
}

void MiniPanWidget::setCwPitch(int pitchHz) {
    if (m_cwPitch != pitchHz) {
        m_cwPitch = pitchHz;
        update();
    }
}

int MiniPanWidget::bandwidthForMode(const QString &mode) const {
    // CW modes use 3 kHz bandwidth (±1.5 kHz)
    if (mode == "CW" || mode == "CW-R") {
        return 3000;
    }
    // All other modes (USB, LSB, DATA, DATA-R, AM, FM) use 10 kHz (±5 kHz)
    return 10000;
}

void MiniPanWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Dark background
    painter.fillRect(rect(), MiniPanColors::DarkBackground);

    int w = width();
    int h = height();

    // 40% spectrum, 60% waterfall (more headroom for spectrum peaks)
    int spectrumHeight = static_cast<int>(h * 0.40f);
    int waterfallHeight = h - spectrumHeight;

    QRect spectrumRect(0, 0, w, spectrumHeight);
    QRect waterfallRect(0, spectrumHeight, w, waterfallHeight);

    // Draw waterfall first
    drawWaterfall(painter, waterfallRect);

    // Draw spectrum
    drawSpectrum(painter, spectrumRect);

    // Draw filter passband in both areas
    drawFilterPassband(painter, spectrumRect);
    drawFilterPassband(painter, waterfallRect);

    // Draw frequency marker (center line) in both areas
    drawFrequencyMarker(painter, spectrumRect);
    drawFrequencyMarker(painter, waterfallRect);

    // Draw notch filter marker in both areas
    drawNotchFilter(painter, spectrumRect);
    drawNotchFilter(painter, waterfallRect);

    // Draw separator line
    painter.setPen(QPen(MiniPanColors::GridLine, 1));
    painter.drawLine(0, spectrumHeight, w, spectrumHeight);

    // Draw border
    painter.setPen(QPen(QColor("#444444"), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

void MiniPanWidget::drawSpectrum(QPainter &painter, const QRect &rect) {
    if (m_smoothedSpectrum.isEmpty())
        return;

    int x0 = rect.left();
    int y0 = rect.top();
    int w = rect.width();
    int h = rect.height();
    int dataSize = m_smoothedSpectrum.size();

    // Scale factor for downsampling (data has more points than display pixels)
    float scale = static_cast<float>(dataSize) / w;

    // Peak-hold downsampling: take max value across all bins mapping to each pixel
    // This ensures no signals are missed when downsampling 1033 bins to 200 pixels
    auto getPeakForPixel = [this, dataSize, scale](int pixelX) {
        int startBin = static_cast<int>(pixelX * scale);
        int endBin = static_cast<int>((pixelX + 1) * scale);
        startBin = qBound(0, startBin, dataSize - 1);
        endBin = qBound(startBin + 1, endBin, dataSize);

        float peak = m_smoothedSpectrum[startBin];
        for (int i = startBin + 1; i < endBin; ++i) {
            if (m_smoothedSpectrum[i] > peak) {
                peak = m_smoothedSpectrum[i];
            }
        }
        return peak;
    };

    // Find baseline for stable display using peak values
    float frameMin = 1.0f;
    for (int x = 0; x < w; ++x) {
        float value = getPeakForPixel(x);
        float normalized = normalizeDb(value);
        if (normalized < frameMin) {
            frameMin = normalized;
        }
    }

    // Smooth the baseline
    const float baselineAlpha = 0.05f;
    if (m_smoothedBaseline < 0.001f) {
        m_smoothedBaseline = frameMin;
    } else {
        m_smoothedBaseline = baselineAlpha * frameMin + (1.0f - baselineAlpha) * m_smoothedBaseline;
    }

    // Build spectrum path using peak values for each pixel
    QPainterPath path;
    for (int x = 0; x < w; ++x) {
        float value = getPeakForPixel(x);
        float normalized = normalizeDb(value);
        float adjusted = normalized - m_smoothedBaseline;
        // Apply height boost for more prominent signal display
        float lineHeight = adjusted * h * 0.95f * m_heightBoost;
        float y = y0 + h - lineHeight;

        if (x == 0) {
            path.moveTo(x0 + x, y);
        } else {
            path.lineTo(x0 + x, y);
        }
    }

    painter.setPen(QPen(m_spectrumColor, 1.0));
    painter.drawPath(path);
}

void MiniPanWidget::drawWaterfall(QPainter &painter, const QRect &rect) {
    int w = rect.width();
    int h = rect.height();

    if (w <= 0 || h <= 0)
        return;

    // Create image at display width × full history (like full panadapter)
    // This ensures consistent display size and smooth stacking from start
    QImage waterfallImage(w, WATERFALL_HISTORY, QImage::Format_RGB32);

    for (int displayRow = 0; displayRow < WATERFALL_HISTORY; ++displayRow) {
        // Map display row to circular buffer: row 0 = newest (writeIndex - 1)
        int histIdx = (m_waterfallWriteIndex - 1 - displayRow + WATERFALL_HISTORY) % WATERFALL_HISTORY;
        const QVector<float> &spectrum = m_waterfallHistory[histIdx];
        QRgb *scanLine = reinterpret_cast<QRgb *>(waterfallImage.scanLine(displayRow));

        if (spectrum.isEmpty()) {
            // Empty row = black (maintains consistent display)
            for (int x = 0; x < w; ++x) {
                scanLine[x] = qRgb(0, 0, 0);
            }
        } else {
            // Average downsampling: average all bins mapping to each pixel for smooth waterfall
            int dataSize = spectrum.size();
            float scale = static_cast<float>(dataSize) / w;

            for (int x = 0; x < w; ++x) {
                int startBin = static_cast<int>(x * scale);
                int endBin = static_cast<int>((x + 1) * scale);
                startBin = qBound(0, startBin, dataSize - 1);
                endBin = qBound(startBin + 1, endBin, dataSize);

                float sum = 0.0f;
                int count = endBin - startBin;
                for (int i = startBin; i < endBin; ++i) {
                    sum += spectrum[i];
                }
                float avgDb = sum / count;
                scanLine[x] = dbToColor(avgDb);
            }
        }
    }

    // Scale height only (width already matches display)
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawImage(rect, waterfallImage);
}

QRgb MiniPanWidget::dbToColor(float db) {
    // Simple linear mapping - no contrast boost needed with correct decompression
    int index = static_cast<int>(normalizeDb(db) * 255.0f);
    index = qBound(0, index, 255);
    return m_colorLUT[index];
}

float MiniPanWidget::normalizeDb(float db) {
    return qBound(0.0f, (db - m_minDb) / (m_maxDb - m_minDb), 1.0f);
}

void MiniPanWidget::drawNotchFilter(QPainter &painter, const QRect &rect) {
    if (!m_notchEnabled || m_notchPitchHz <= 0)
        return;

    // MiniPan bandwidth is mode-dependent: CW=3kHz, Voice/Data=10kHz
    // Notch pitch is audio offset from carrier:
    // USB: notch is to the right of center (positive offset)
    // LSB: notch is to the left of center (negative offset)
    int offsetHz;
    if (m_mode == "LSB") {
        offsetHz = -m_notchPitchHz; // LSB: below carrier = left of center
    } else {
        offsetHz = m_notchPitchHz; // USB/CW/DATA: above carrier = right of center
    }

    // Convert audio offset to pixel position
    // Center of display = 0 Hz offset
    // Full width = m_bandwidthHz (mode-dependent)
    int centerX = rect.width() / 2;
    int notchX = centerX + static_cast<int>((static_cast<float>(offsetHz) / m_bandwidthHz) * rect.width());

    // Only draw if within visible range
    if (notchX >= 0 && notchX < rect.width()) {
        painter.setPen(QPen(QColor("#FF0000"), 2.0)); // Red, 2px wide
        painter.drawLine(notchX, rect.top(), notchX, rect.bottom());
    }
}

void MiniPanWidget::drawFilterPassband(QPainter &painter, const QRect &rect) {
    if (m_filterBw <= 0)
        return;

    int w = rect.width();
    int centerX = w / 2; // Mini-pan is centered on VFO

    // Convert filter bandwidth to pixels
    int bwPixels = (m_filterBw * w) / m_bandwidthHz;

    // Calculate shift offset for CW modes
    // K4 IF shift: IS0050 displayed as "0.50" means 500 Hz
    // Value is in units of 10 Hz, so IS0050 = 500 Hz
    // Shift offset = (shift_value - pitch_value) * 10 Hz
    int shiftHz = m_ifShift * 10;
    int shiftOffsetHz = shiftHz - m_cwPitch;
    int shiftPixels = (shiftOffsetHz * w) / m_bandwidthHz;

    // Calculate passband position based on mode
    int passbandX;
    if (m_mode == "CW" || m_mode == "CW-R") {
        // CW modes: filter is centered on VFO marker when shift equals pitch
        // Shift moves the passband left/right from center
        passbandX = centerX + shiftPixels - bwPixels / 2;
    } else if (m_mode == "LSB") {
        // LSB: filter is below carrier (left of center)
        passbandX = centerX - bwPixels;
    } else {
        // USB, DATA, DATA-R, AM, FM: filter is above carrier (right of center)
        passbandX = centerX;
    }

    // Draw semi-transparent passband overlay (use member color with 25% opacity)
    QColor fillColor = m_passbandColor;
    fillColor.setAlpha(64);
    painter.fillRect(passbandX, rect.top(), bwPixels, rect.height(), fillColor);
}

void MiniPanWidget::drawFrequencyMarker(QPainter &painter, const QRect &rect) {
    // Draw vertical line at center (Mini-Pan is always centered on VFO frequency)
    int centerX = rect.width() / 2;

    // Use darker version of passband color for the marker
    QColor markerColor = m_passbandColor;
    markerColor.setAlpha(255); // Full opacity
    // Darken the color
    markerColor = markerColor.darker(150);

    painter.setPen(QPen(markerColor, 2));
    painter.drawLine(centerX, rect.top(), centerX, rect.bottom());
}

void MiniPanWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked();
        event->accept();
    } else {
        QWidget::mousePressEvent(event);
    }
}
