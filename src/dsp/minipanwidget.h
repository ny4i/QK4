#ifndef MINIPANWIDGET_H
#define MINIPANWIDGET_H

#include <QWidget>
#include <QVector>
#include <QColor>

// Compact Mini-Pan widget for VFO area
// Shows simplified spectrum + waterfall in a small form factor
// Fixed ±1kHz span around VFO frequency
class MiniPanWidget : public QWidget {
    Q_OBJECT

public:
    explicit MiniPanWidget(QWidget *parent = nullptr);
    ~MiniPanWidget() = default;

    // Update spectrum from MiniPAN packet (TYPE=3)
    // bins: raw compressed data (dB = byte * 10, then offset)
    void updateSpectrum(const QByteArray &bins);
    void clear();

    // Spectrum line color (default blue for VFO A, green for VFO B)
    void setSpectrumColor(const QColor &color);

    // Passband color (default blue for VFO A, green for VFO B)
    void setPassbandColor(const QColor &color);

    // Notch filter visualization
    void setNotchFilter(bool enabled, int pitchHz);
    void setMode(const QString &mode);

    // Filter passband visualization
    void setFilterBandwidth(int bwHz);
    void setIfShift(int shift);
    void setCwPitch(int pitchHz);

signals:
    void clicked(); // Emitted when user clicks to toggle back to normal view

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void initColorLUT();
    void drawSpectrum(QPainter &painter, const QRect &rect);
    void drawWaterfall(QPainter &painter, const QRect &rect);
    void drawNotchFilter(QPainter &painter, const QRect &rect);
    void drawFilterPassband(QPainter &painter, const QRect &rect);
    void drawFrequencyMarker(QPainter &painter, const QRect &rect);
    QRgb dbToColor(float db);
    float normalizeDb(float db);

    // Spectrum data
    QVector<float> m_spectrum;         // Current spectrum in dB
    QVector<float> m_smoothedSpectrum; // Smoothed for display

    // Waterfall data (circular buffer)
    QVector<QVector<float>> m_waterfallHistory;
    int m_waterfallWriteIndex = 0;
    static const int WATERFALL_HISTORY = 100; // More history for taller display (was 80)

    // Color lookup table
    QVector<QRgb> m_colorLUT;

    // Spectrum line color (customizable for VFO A vs B)
    QColor m_spectrumColor = QColor("#FFB000"); // Default amber (VFO A)

    // Passband color (customizable for VFO A vs B)
    QColor m_passbandColor = QColor(0, 128, 255, 64); // Default blue (VFO A)

    // Display settings - range for Mini-Pan relative data
    // Observed byte range 0-24+ (after /10: 0-2.5+)
    float m_minDb = -1.0f;         // Below 0 for noise floor headroom
    float m_maxDb = 4.0f;          // Above max expected ~2.5-3.0 for strong signals
    float m_smoothingAlpha = 0.3f; // Faster smoothing for responsive display
    float m_smoothedBaseline = 0.0f;
    float m_heightBoost = 1.5f; // Amplify spectrum height (1.0 = no boost)

    // Legacy constant - bandwidth is now mode-dependent (see m_bandwidthHz)
    static constexpr int TOTAL_BANDWIDTH_HZ = 3000; // CW only: ±1.5 kHz

    // Notch filter visualization
    bool m_notchEnabled = false;
    int m_notchPitchHz = 0;
    QString m_mode = "USB";
    int m_bandwidthHz = 10000; // Mode-dependent span: CW=3kHz, Voice/Data=10kHz

    // Filter passband visualization
    int m_filterBw = 2400; // Filter bandwidth in Hz
    int m_ifShift = 50;    // IF shift value (0-99, 50=center)
    int m_cwPitch = 600;   // CW sidetone pitch Hz

    // Helper to determine bandwidth from mode
    int bandwidthForMode(const QString &mode) const;
};

#endif // MINIPANWIDGET_H
