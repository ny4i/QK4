#include "panadapter.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QLinearGradient>
#include <QRadialGradient>
#include <cmath>

// K4 Color scheme (matching mainwindow.cpp)
namespace K4Colors {
const QColor Background("#1a1a1a");
const QColor DarkBackground("#0d0d0d");
const QColor VfoAAmber("#FFB000");
const QColor VfoBCyan("#00BFFF");
const QColor TxRed("#FF0000");
const QColor AgcGreen("#00FF00");
const QColor InactiveGray("#666666");
const QColor TextWhite("#FFFFFF");
const QColor TextGray("#999999");
const QColor GridLine("#333333");
} // namespace K4Colors

PanadapterWidget::PanadapterWidget(QWidget *parent)
    : QWidget(parent), m_waterfallWriteIndex(0), m_centerFreq(0), m_sampleRate(192000), m_noiseFloor(-130.0f),
      m_tunedFreq(0), m_filterBw(2400), m_mode("USB"), m_minDb(-138.0f) // REF-28 for default REF=-110
      ,
      m_maxDb(-58.0f) // REF+52 for default REF=-110 (80 dB range)
      ,
      m_spectrumRatio(0.30f) // 30% spectrum, 70% waterfall (matching K4Mobile)
      ,
      m_smoothingAlpha(0.80f) // Higher = less averaging, more responsive
      ,
      m_spectrumColor(K4Colors::VfoAAmber), m_gridEnabled(true), m_peakHoldEnabled(true),
      m_refLevel(-110) // Default REF level from K4
      ,
      m_spanHz(10000) // Default 10 kHz span (from #SPN command)
      ,
      m_peakDecayTimer(new QTimer(this)), m_waterfallMarkerTimer(new QTimer(this)), m_showWaterfallMarker(false) {
    setMinimumHeight(200);
    setMouseTracking(true);

    initColorLUT();

    // Initialize waterfall circular buffer
    m_waterfallHistory.resize(WATERFALL_HISTORY);

    // Peak hold decay timer
    connect(m_peakDecayTimer, &QTimer::timeout, this, [this]() {
        if (!m_peakHold.isEmpty()) {
            for (int i = 0; i < m_peakHold.size(); ++i) {
                m_peakHold[i] -= PEAK_DECAY_RATE;
                if (m_peakHold[i] < m_currentSpectrum.value(i, m_minDb)) {
                    m_peakHold[i] = m_currentSpectrum.value(i, m_minDb);
                }
            }
            update();
        }
    });
    m_peakDecayTimer->start(50); // 20 Hz decay rate

    // Waterfall marker timer - hides marker in waterfall after tuning stops
    m_waterfallMarkerTimer->setSingleShot(true);
    connect(m_waterfallMarkerTimer, &QTimer::timeout, this, [this]() {
        m_showWaterfallMarker = false;
        update();
    });
}

PanadapterWidget::~PanadapterWidget() {}

void PanadapterWidget::initColorLUT() {
    // Create 256-entry color LUT for waterfall
    // 8-stage color progression matching K4Mobile:
    // Black → Dark Blue → Royal Blue → Cyan → Green → Yellow → Red
    m_colorLUT.resize(256);

    for (int i = 0; i < 256; ++i) {
        float value = i / 255.0f;
        int r, g, b;

        if (value < 0.10f) {
            // Stage 1: Black (noise floor)
            r = 0;
            g = 0;
            b = 0;
        } else if (value < 0.25f) {
            // Stage 2: Black → Dark Blue
            float t = (value - 0.10f) / 0.15f;
            r = 0;
            g = 0;
            b = static_cast<int>(t * 51); // 0.2 * 255
        } else if (value < 0.40f) {
            // Stage 3: Dark Blue → Royal Blue
            float t = (value - 0.25f) / 0.15f;
            r = 0;
            g = 0;
            b = static_cast<int>(51 + t * 102); // 0.2 + 0.4 * 255
        } else if (value < 0.55f) {
            // Stage 4: Royal Blue → Cyan
            float t = (value - 0.40f) / 0.15f;
            r = 0;
            g = static_cast<int>(t * 128);       // 0 → 0.5
            b = static_cast<int>(153 + t * 102); // 0.6 → 1.0
        } else if (value < 0.70f) {
            // Stage 5: Cyan → Green
            float t = (value - 0.55f) / 0.15f;
            r = 0;
            g = static_cast<int>(128 + t * 127);    // 0.5 → 1.0
            b = static_cast<int>(255 * (1.0f - t)); // 1.0 → 0
        } else if (value < 0.85f) {
            // Stage 6: Green → Yellow
            float t = (value - 0.70f) / 0.15f;
            r = static_cast<int>(t * 255);
            g = 255;
            b = 0;
        } else {
            // Stage 7: Yellow → Red
            float t = (value - 0.85f) / 0.15f;
            r = 255;
            g = static_cast<int>(255 * (1.0f - t));
            b = 0;
        }

        m_colorLUT[i] = qRgb(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255));
    }
}

void PanadapterWidget::updateSpectrum(const QByteArray &bins, qint64 centerFreq, qint32 sampleRate, float noiseFloor) {
    m_centerFreq = centerFreq;
    m_sampleRate = sampleRate;
    m_noiseFloor = noiseFloor;

    // K4 tier span = sampleRate * 1000 Hz (24kHz, 48kHz, 96kHz, etc.)
    qint32 tierSpanHz = sampleRate * 1000;
    int totalBins = bins.size();

    // Extract center bins if tier span > commanded span
    // K4 sends full tier data, we only display the portion matching #SPN span
    QByteArray binsToUse;
    if (tierSpanHz > m_spanHz && totalBins > 100 && m_spanHz > 0) {
        // Calculate how many bins correspond to commanded span
        int requestedBins = (static_cast<qint64>(m_spanHz) * totalBins) / tierSpanHz;
        requestedBins = qBound(50, requestedBins, totalBins);

        // Extract center portion
        int centerStart = (totalBins - requestedBins) / 2;
        binsToUse = bins.mid(centerStart, requestedBins);
    } else {
        binsToUse = bins;
    }

    // Decompress bins to dB values
    decompressBins(binsToUse, m_rawSpectrum);

    // Apply EMA smoothing
    applySmoothing(m_rawSpectrum, m_currentSpectrum);

    // Update peak hold
    if (m_peakHoldEnabled) {
        if (m_peakHold.size() != m_currentSpectrum.size()) {
            m_peakHold = m_currentSpectrum;
        } else {
            for (int i = 0; i < m_currentSpectrum.size(); ++i) {
                if (m_currentSpectrum[i] > m_peakHold[i]) {
                    m_peakHold[i] = m_currentSpectrum[i];
                }
            }
        }
    }

    // Update waterfall
    updateWaterfall(m_currentSpectrum);

    update();
}

void PanadapterWidget::updateMiniSpectrum(const QByteArray &bins) {
    // MiniPAN uses simpler decompression: dB = byte * 10
    m_rawSpectrum.resize(bins.size());
    for (int i = 0; i < bins.size(); ++i) {
        m_rawSpectrum[i] = static_cast<quint8>(bins[i]) * 10.0f - 160.0f;
    }

    applySmoothing(m_rawSpectrum, m_currentSpectrum);
    updateWaterfall(m_currentSpectrum);
    update();
}

void PanadapterWidget::decompressBins(const QByteArray &bins, QVector<float> &out) {
    // K4 PAN compression: dB = byte_value - 160
    out.resize(bins.size());
    for (int i = 0; i < bins.size(); ++i) {
        out[i] = static_cast<quint8>(bins[i]) - 160.0f;
    }
}

void PanadapterWidget::applySmoothing(const QVector<float> &input, QVector<float> &output) {
    // Exponential Moving Average smoothing
    if (output.size() != input.size()) {
        output = input;
        return;
    }

    for (int i = 0; i < input.size(); ++i) {
        output[i] = m_smoothingAlpha * input[i] + (1.0f - m_smoothingAlpha) * output[i];
    }
}

void PanadapterWidget::upsampleSpectrum(const QVector<float> &input, QVector<float> &output, int targetWidth) {
    // Linear interpolation to match display width for smooth rendering
    if (input.isEmpty() || targetWidth <= 0) {
        output = input;
        return;
    }

    // If input already has enough resolution, just copy
    if (input.size() >= targetWidth) {
        output = input;
        return;
    }

    output.resize(targetWidth);
    float scale = static_cast<float>(input.size() - 1) / (targetWidth - 1);

    for (int i = 0; i < targetWidth; ++i) {
        float srcPos = i * scale;
        int idx = qRound(srcPos); // Nearest-neighbor for sharp peaks
        idx = qBound(0, idx, input.size() - 1);
        output[i] = input[idx];
    }
}

void PanadapterWidget::updateWaterfall(const QVector<float> &spectrum) {
    if (spectrum.isEmpty())
        return;

    // Add spectrum to circular buffer (newest at current write index)
    m_waterfallHistory[m_waterfallWriteIndex] = spectrum;

    // Advance write index (wraps around)
    m_waterfallWriteIndex = (m_waterfallWriteIndex + 1) % WATERFALL_HISTORY;

    // Mark image as needing rebuild
    m_waterfallImage = QImage();
}

QRgb PanadapterWidget::dbToColor(float db) {
    int index = static_cast<int>(normalizeDb(db) * 255.0f);
    index = qBound(0, index, 255);
    return m_colorLUT[index];
}

float PanadapterWidget::normalizeDb(float db) {
    // Normalize dB to 0.0-1.0 range
    return qBound(0.0f, (db - m_minDb) / (m_maxDb - m_minDb), 1.0f);
}

void PanadapterWidget::setDbRange(float minDb, float maxDb) {
    m_minDb = minDb;
    m_maxDb = maxDb;
    update();
}

void PanadapterWidget::setSpectrumRatio(float ratio) {
    m_spectrumRatio = qBound(0.1f, ratio, 0.9f);
    update();
}

void PanadapterWidget::setTunedFrequency(qint64 freq) {
    if (m_tunedFreq != freq) {
        m_tunedFreq = freq;
        m_showWaterfallMarker = true;       // Show marker in waterfall while tuning
        m_waterfallMarkerTimer->start(500); // Hide after 500ms of no changes
        update();
    }
}

void PanadapterWidget::setFilterBandwidth(int bwHz) {
    m_filterBw = bwHz;
    update();
}

void PanadapterWidget::setMode(const QString &mode) {
    m_mode = mode;
    update();
}

void PanadapterWidget::setIfShift(int shift) {
    if (m_ifShift != shift) {
        m_ifShift = shift;
        update();
    }
}

void PanadapterWidget::setCwPitch(int pitchHz) {
    if (m_cwPitch != pitchHz) {
        m_cwPitch = pitchHz;
        update();
    }
}

void PanadapterWidget::setSpectrumColor(const QColor &color) {
    m_spectrumColor = color;
    update();
}

void PanadapterWidget::setGridEnabled(bool enabled) {
    m_gridEnabled = enabled;
    update();
}

void PanadapterWidget::setPeakHoldEnabled(bool enabled) {
    m_peakHoldEnabled = enabled;
    if (!enabled) {
        m_peakHold.clear();
    }
    update();
}

void PanadapterWidget::setRefLevel(int level) {
    if (m_refLevel != level) {
        m_refLevel = level;
        // Update dB range based on REF level (K4Mobile pattern)
        // REF level is noise floor, mapped to ~35% of color gradient
        // 80 dB total range: [REF-28, REF+52]
        m_minDb = static_cast<float>(level - 28);
        m_maxDb = static_cast<float>(level + 52);
        update();
    }
}

void PanadapterWidget::setSpan(int spanHz) {
    if (m_spanHz != spanHz && spanHz > 0) {
        m_spanHz = spanHz;
        update();
    }
}

void PanadapterWidget::setNotchFilter(bool enabled, int pitchHz) {
    if (m_notchEnabled != enabled || m_notchPitchHz != pitchHz) {
        m_notchEnabled = enabled;
        m_notchPitchHz = pitchHz;
        update();
    }
}

void PanadapterWidget::clear() {
    m_currentSpectrum.clear();
    m_rawSpectrum.clear();
    m_peakHold.clear();
    m_waterfallImage = QImage();
    m_waterfallWriteIndex = 0;
    // Clear all history rows
    for (int i = 0; i < m_waterfallHistory.size(); ++i) {
        m_waterfallHistory[i].clear();
    }
    update();
}

void PanadapterWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background - radial gradient from darker edges to lighter center
    QRadialGradient gradient(rect().center(), qMax(rect().width(), rect().height()) / 2.0);
    gradient.setColorAt(0.0, QColor(61, 61, 61)); // Center - lighter
    gradient.setColorAt(1.0, QColor(47, 47, 47)); // Edges - darker
    painter.fillRect(rect(), gradient);

    int w = width();
    int h = height();

    // Calculate regions - 30% spectrum, 70% waterfall (matching K4Mobile)
    int spectrumHeight = static_cast<int>(h * m_spectrumRatio);
    int waterfallHeight = h - spectrumHeight;

    QRect spectrumRect(0, 0, w, spectrumHeight);
    QRect waterfallRect(0, spectrumHeight, w, waterfallHeight);

    // Draw waterfall first (behind spectrum)
    drawWaterfall(painter, waterfallRect);

    // Draw spectrum
    drawSpectrum(painter, spectrumRect);

    // Draw grid overlay (panadapter area only)
    if (m_gridEnabled) {
        drawGrid(painter, spectrumRect);
    }

    // Draw filter passband and frequency marker in spectrum area (only when connected/receiving data)
    if (m_centerFreq > 0) {
        drawFilterPassband(painter, spectrumRect);
        drawFrequencyMarker(painter, spectrumRect);
        drawNotchFilter(painter, spectrumRect);
    }

    // Draw filter passband and frequency marker in waterfall area (only while tuning)
    if (m_showWaterfallMarker && m_centerFreq > 0) {
        drawFilterPassband(painter, waterfallRect);
        drawFrequencyMarker(painter, waterfallRect);
        drawNotchFilter(painter, waterfallRect);
    }

    // Draw separator line between spectrum and waterfall
    painter.setPen(QPen(K4Colors::GridLine, 1.5));
    painter.drawLine(0, spectrumHeight, w, spectrumHeight);
}

void PanadapterWidget::drawSpectrum(QPainter &painter, const QRect &rect) {
    if (m_currentSpectrum.isEmpty())
        return;

    int x0 = rect.left();
    int y0 = rect.top();
    int w = rect.width();
    int h = rect.height();

    // Upsample spectrum to match display width for smooth rendering
    QVector<float> displaySpectrum;
    upsampleSpectrum(m_currentSpectrum, displaySpectrum, w);

    // Find current frame's minimum normalized value (noise floor)
    float frameMinNormalized = 1.0f;
    for (int x = 0; x < w; ++x) {
        float normalized = normalizeDb(displaySpectrum[x]);
        if (normalized < frameMinNormalized) {
            frameMinNormalized = normalized;
        }
    }

    // Smooth the baseline using EMA to prevent bouncing
    // This ensures stable spectrum display while still adapting to band changes
    const float baselineAlpha = 0.05f; // Slow smoothing for stability
    if (m_smoothedBaseline < 0.001f) {
        m_smoothedBaseline = frameMinNormalized; // Initialize on first frame
    } else {
        m_smoothedBaseline = baselineAlpha * frameMinNormalized + (1.0f - baselineAlpha) * m_smoothedBaseline;
    }

    // Build spectrum path with smoothed baseline correction
    QPainterPath path;
    float heightScale = 0.95f; // Use 95% of height for headroom at top

    for (int x = 0; x < w; ++x) {
        float normalized = normalizeDb(displaySpectrum[x]);
        // Subtract smoothed baseline so noise floor maps to 0 (sits at demarcation line)
        // Clamp to non-negative to prevent spectrum bleeding into waterfall area
        float adjusted = qMax(0.0f, normalized - m_smoothedBaseline);
        float lineHeight = adjusted * h * heightScale;
        float y = y0 + h - lineHeight;

        if (x == 0) {
            path.moveTo(x0 + x, y);
        } else {
            path.lineTo(x0 + x, y);
        }
    }

    // Create filled path (close the path along the bottom edge of spectrum area)
    QPainterPath fillPath = path;
    fillPath.lineTo(x0 + w - 1, y0 + h); // Bottom right corner
    fillPath.lineTo(x0, y0 + h);         // Bottom left corner
    fillPath.closeSubpath();

    // Gradient fill: bright lime at top, richer green at base
    QLinearGradient gradient(0, y0, 0, y0 + h);
    gradient.setColorAt(0.0, QColor(140, 255, 140, 230)); // Bright lime at top
    gradient.setColorAt(0.4, QColor(60, 220, 60, 200));   // Strong green
    gradient.setColorAt(0.7, QColor(30, 180, 30, 170));   // Medium green
    gradient.setColorAt(1.0, QColor(0, 140, 0, 150));     // Deep green base, more opaque

    painter.fillPath(fillPath, QBrush(gradient));

    // Draw spectrum line (lime green)
    painter.setPen(QPen(QColor("#32FF32"), 1.0));
    painter.drawPath(path);

    // Draw peak hold if enabled (filled gradient shadow)
    if (m_peakHoldEnabled && !m_peakHold.isEmpty()) {
        QVector<float> displayPeakHold;
        upsampleSpectrum(m_peakHold, displayPeakHold, w);

        QPainterPath peakPath;
        peakPath.moveTo(x0, y0 + h); // Start at bottom-left

        for (int x = 0; x < w; ++x) {
            float normalized = normalizeDb(displayPeakHold[x]);
            // Use same smoothed baseline correction for peak hold
            // Clamp to non-negative to prevent bleeding into waterfall area
            float adjusted = qMax(0.0f, normalized - m_smoothedBaseline);
            float lineHeight = adjusted * h * heightScale;
            float y = y0 + h - lineHeight;
            peakPath.lineTo(x0 + x, y);
        }

        // Close the path back to baseline
        peakPath.lineTo(x0 + w - 1, y0 + h);
        peakPath.closeSubpath();

        // White gradient fill (fades from top to bottom)
        QLinearGradient peakGradient(0, y0, 0, y0 + h);
        peakGradient.setColorAt(0.0, QColor(255, 255, 255, 140)); // White at top
        peakGradient.setColorAt(0.5, QColor(255, 255, 255, 70));  // Fade
        peakGradient.setColorAt(1.0, QColor(255, 255, 255, 0));   // Transparent at bottom

        painter.setPen(Qt::NoPen);
        painter.setBrush(peakGradient);
        painter.drawPath(peakPath);
    }
}

void PanadapterWidget::drawWaterfall(QPainter &painter, const QRect &rect) {
    // Build waterfall image from circular buffer
    // Upsample each row to display width for smooth rendering at any size
    int displayWidth = rect.width();

    // Find bin count from first non-empty row
    int binCount = 0;
    for (int i = 0; i < WATERFALL_HISTORY; ++i) {
        if (!m_waterfallHistory[i].isEmpty()) {
            binCount = m_waterfallHistory[i].size();
            break;
        }
    }

    if (binCount == 0) {
        painter.fillRect(rect, Qt::black);
        return;
    }

    // Create image at display width for crisp rendering
    // Each row is upsampled from bin data to display pixels
    m_waterfallImage = QImage(displayWidth, WATERFALL_HISTORY, QImage::Format_RGB32);

    for (int displayRow = 0; displayRow < WATERFALL_HISTORY; ++displayRow) {
        // Map display row to circular buffer index
        // displayRow 0 = top of waterfall = newest data = writeIndex - 1
        // displayRow 255 = bottom = oldest data = writeIndex
        int bufferIndex = (m_waterfallWriteIndex - 1 - displayRow + WATERFALL_HISTORY) % WATERFALL_HISTORY;

        const QVector<float> &row = m_waterfallHistory[bufferIndex];
        QRgb *scanLine = reinterpret_cast<QRgb *>(m_waterfallImage.scanLine(displayRow));

        if (row.isEmpty()) {
            // Empty row = black
            for (int x = 0; x < displayWidth; ++x) {
                scanLine[x] = qRgb(0, 0, 0);
            }
        } else {
            // Upsample row to display width for smooth rendering
            QVector<float> upsampledRow;
            upsampleSpectrum(row, upsampledRow, displayWidth);

            // Convert upsampled dB values to colors
            for (int x = 0; x < displayWidth; ++x) {
                scanLine[x] = dbToColor(upsampledRow[x]);
            }
        }
    }

    // Draw waterfall image (already at display width, just scale height)
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawImage(rect, m_waterfallImage);
}

void PanadapterWidget::drawGrid(QPainter &painter, const QRect &rect) {
    painter.setPen(QPen(QColor(255, 255, 255, 38), 1)); // 15% opacity white

    int w = rect.width();
    int h = rect.height();

    // Finer vertical grid - approximately one line every 50 pixels, minimum 12 divisions
    int vertDivisions = qMax(12, w / 50);
    for (int i = 0; i <= vertDivisions; ++i) {
        int x = (w * i) / vertDivisions;
        painter.drawLine(x, rect.top(), x, rect.bottom());
    }

    // Finer horizontal grid - approximately one line every 40 pixels, minimum 8 divisions
    int horzDivisions = qMax(8, h / 40);
    for (int i = 0; i <= horzDivisions; ++i) {
        int y = rect.top() + (h * i) / horzDivisions;
        painter.drawLine(0, y, w, y);
    }

    // Calculate frequency values using PAN center frequency
    qint64 startFreq = m_centerFreq - m_spanHz / 2;
    qint64 endFreq = m_centerFreq + m_spanHz / 2;

    // Draw REF level indicator (top-left, yellow)
    QFont labelFont = font();
    labelFont.setPointSize(10);
    painter.setFont(labelFont);
    painter.setPen(QColor("#FFFF00")); // Yellow
    QString refStr = QString("REF %1 dB").arg(m_refLevel);
    painter.drawText(4, rect.top() + 14, refStr);

    // Draw semi-transparent frequency label bar at bottom (K4Mobile pattern)
    QRect labelBar(0, height() - 20, w, 20);
    painter.fillRect(labelBar, QColor(0, 0, 0, 178)); // 70% opacity black

    labelFont.setPointSize(10);
    painter.setFont(labelFont);

    // Left frequency (white 80% opacity)
    painter.setPen(QColor(255, 255, 255, 204));
    double startMHz = startFreq / 1000000.0;
    QString startStr = QString::number(startMHz, 'f', 3);
    painter.drawText(4, height() - 5, startStr);

    // Center frequency (orange, bold) - use PAN center frequency
    QFont boldFont = labelFont;
    boldFont.setBold(true);
    painter.setFont(boldFont);
    painter.setPen(QColor("#FF8C00")); // Orange
    double centerMHz = m_centerFreq / 1000000.0;
    QString centerStr = QString::number(centerMHz, 'f', 3);
    int centerWidth = painter.fontMetrics().horizontalAdvance(centerStr);
    painter.drawText((w - centerWidth) / 2, height() - 5, centerStr);

    // Right frequency (white 80% opacity)
    painter.setFont(labelFont);
    painter.setPen(QColor(255, 255, 255, 204));
    double endMHz = endFreq / 1000000.0;
    QString endStr = QString::number(endMHz, 'f', 3);
    int endWidth = painter.fontMetrics().horizontalAdvance(endStr);
    painter.drawText(w - endWidth - 4, height() - 5, endStr);
}

void PanadapterWidget::drawFrequencyMarker(QPainter &painter, const QRect &rect) {
    // Draw vertical line at tuned frequency (panadapter section only)
    int x = freqToX(m_tunedFreq, rect.width());
    if (x >= 0 && x < rect.width()) {
        painter.setPen(QPen(QColor("#FFA500"), 2)); // Orange
        painter.drawLine(x, rect.top(), x, rect.bottom());
    }
}

void PanadapterWidget::drawFilterPassband(QPainter &painter, const QRect &rect) {
    if (m_filterBw <= 0)
        return;

    int carrierX = freqToX(m_tunedFreq, rect.width());

    // Convert bandwidth to pixels
    int bwPixels = static_cast<int>((static_cast<double>(m_filterBw) / m_spanHz) * rect.width());

    // Calculate IF shift offset in Hz
    // K4 IF shift: IS0050 displayed as "0.50" means 500 Hz
    // Value is in units of 10 Hz, so IS0050 = 500 Hz
    // Center position (no shift) is when displayed value equals pitch (e.g., 0.50 = 500 Hz)
    // Shift offset = (shift_value - pitch_value) * 10 Hz
    // When shift equals pitch, filter is centered on VFO
    int shiftHz = m_ifShift * 10;            // Convert IS value to Hz
    int shiftOffsetHz = shiftHz - m_cwPitch; // Offset from center (pitch)
    int shiftPixels = static_cast<int>((static_cast<double>(shiftOffsetHz) / m_spanHz) * rect.width());

    // Calculate passband position based on mode
    int passbandX;

    if (m_mode == "CW" || m_mode == "CW-R") {
        // CW modes: filter is centered on VFO marker when shift equals pitch
        // Shift moves the passband left/right from center
        int centerX = carrierX + shiftPixels;
        passbandX = centerX - bwPixels / 2;
    } else if (m_mode == "LSB") {
        // LSB: filter is below carrier
        passbandX = carrierX - bwPixels;
    } else {
        // USB, DATA, DATA-R, AM, FM: filter is above carrier
        passbandX = carrierX;
    }

    // Draw semi-transparent green passband overlay (25% opacity like K4Mobile)
    QColor passbandColor(0, 255, 0, 64); // 25% opacity green
    painter.fillRect(passbandX, rect.top(), bwPixels, rect.height(), passbandColor);

    // Draw passband edges
    painter.setPen(QPen(QColor(0, 255, 0, 153), 1.5)); // 60% opacity green
    painter.drawLine(passbandX, rect.top(), passbandX, rect.bottom());
    painter.drawLine(passbandX + bwPixels, rect.top(), passbandX + bwPixels, rect.bottom());
}

void PanadapterWidget::drawNotchFilter(QPainter &painter, const QRect &rect) {
    if (!m_notchEnabled || m_notchPitchHz <= 0)
        return;

    // Calculate RF frequency from audio pitch
    // USB/DATA: notch is above carrier (add pitch)
    // LSB: notch is below carrier (subtract pitch)
    qint64 notchFreq;
    if (m_mode == "LSB") {
        notchFreq = m_tunedFreq - m_notchPitchHz;
    } else {
        // USB, DATA, DATA-R, CW, CW-R, AM, FM
        notchFreq = m_tunedFreq + m_notchPitchHz;
    }

    int notchX = freqToX(notchFreq, rect.width());
    if (notchX >= 0 && notchX < rect.width()) {
        // Draw red vertical line for notch filter
        painter.setPen(QPen(QColor("#FF0000"), 2.0)); // Red, 2px wide
        painter.drawLine(notchX, rect.top(), notchX, rect.bottom());
    }
}

int PanadapterWidget::freqToX(qint64 freq, int w) {
    // Use PAN packet center frequency for display positioning
    qint64 startFreq = m_centerFreq - m_spanHz / 2;
    qint64 endFreq = m_centerFreq + m_spanHz / 2;

    if (freq < startFreq || freq > endFreq) {
        // Allow off-screen but clamp for drawing
        if (freq < startFreq)
            return -10;
        return w + 10;
    }

    return static_cast<int>((static_cast<double>(freq - startFreq) / m_spanHz) * w);
}

qint64 PanadapterWidget::xToFreq(int x, int w) {
    // Use PAN packet center frequency for display positioning
    qint64 startFreq = m_centerFreq - m_spanHz / 2;

    return startFreq + (static_cast<qint64>(x) * m_spanHz) / w;
}

void PanadapterWidget::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event)
    // Waterfall image will be recreated on next update with new dimensions
}

void PanadapterWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        qint64 freq = xToFreq(event->pos().x(), width());
        emit frequencyClicked(freq);
    }
}

void PanadapterWidget::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        qint64 freq = xToFreq(event->pos().x(), width());
        emit frequencyDragged(freq);
    }
}

void PanadapterWidget::wheelEvent(QWheelEvent *event) {
    // Standard mouse wheel: 120 units per "click"
    int degrees = event->angleDelta().y() / 8;
    int steps = degrees / 15; // Each step = 15 degrees

    if (steps != 0) {
        emit frequencyScrolled(steps);
    }
    event->accept();
}
