#ifndef MINIPANWIDGET_H
#define MINIPANWIDGET_H

#include <QWidget>
#include <QVector>
#include <QColor>

// Compact Mini-Pan widget for VFO area
// Shows simplified spectrum + waterfall in a small form factor
// Fixed ±1kHz span around VFO frequency
class MiniPanWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MiniPanWidget(QWidget *parent = nullptr);
    ~MiniPanWidget() = default;

    // Update spectrum from MiniPAN packet (TYPE=3)
    // bins: raw compressed data (dB = byte * 10, then offset)
    void updateSpectrum(const QByteArray &bins);
    void clear();

    // Notch filter visualization
    void setNotchFilter(bool enabled, int pitchHz);
    void setMode(const QString &mode);

signals:
    void clicked();  // Emitted when user clicks to toggle back to normal view

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void initColorLUT();
    void drawSpectrum(QPainter &painter, const QRect &rect);
    void drawWaterfall(QPainter &painter, const QRect &rect);
    void drawNotchFilter(QPainter &painter, const QRect &rect);
    QRgb dbToColor(float db);
    float normalizeDb(float db);

    // Spectrum data
    QVector<float> m_spectrum;           // Current spectrum in dB
    QVector<float> m_smoothedSpectrum;   // Smoothed for display

    // Waterfall data (circular buffer)
    QVector<QVector<float>> m_waterfallHistory;
    int m_waterfallWriteIndex = 0;
    static const int WATERFALL_HISTORY = 80;  // More history for taller display

    // Color lookup table
    QVector<QRgb> m_colorLUT;

    // Display settings - tight range for Mini-Pan relative data
    // Observed byte range 0-24+ (after /10: 0-2.5+)
    float m_minDb = -0.5f;   // Slightly below 0 for noise floor headroom
    float m_maxDb = 3.5f;    // Slightly above max expected ~2.5-3.0
    float m_smoothingAlpha = 0.3f;  // Faster smoothing for responsive display
    float m_smoothedBaseline = 0.0f;

    // Legacy constant - bandwidth is now mode-dependent (see m_bandwidthHz)
    static constexpr int TOTAL_BANDWIDTH_HZ = 3000;  // CW only: ±1.5 kHz

    // Notch filter visualization
    bool m_notchEnabled = false;
    int m_notchPitchHz = 0;
    QString m_mode = "USB";
    int m_bandwidthHz = 10000;  // Mode-dependent: CW=3kHz, Voice/Data=10kHz

    // Helper to determine bandwidth from mode
    int bandwidthForMode(const QString &mode) const;
};

#endif // MINIPANWIDGET_H
