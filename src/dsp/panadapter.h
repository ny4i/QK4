#ifndef PANADAPTER_H
#define PANADAPTER_H

#include <QWidget>
#include <QImage>
#include <QVector>
#include <QColor>
#include <QTimer>

// Combined panadapter (spectrum) and waterfall display widget
// Based on K4Mobile's SpectrumView pattern
class PanadapterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PanadapterWidget(QWidget *parent = nullptr);
    ~PanadapterWidget();

    // Update spectrum data from K4 PAN packet
    // bins: raw compressed data (dB = byte - 160)
    void updateSpectrum(const QByteArray &bins, qint64 centerFreq, qint32 sampleRate, float noiseFloor);

    // Update from MiniPAN packet (simpler format)
    void updateMiniSpectrum(const QByteArray &bins);

    // Configuration
    void setDbRange(float minDb, float maxDb);
    void setSpectrumRatio(float ratio); // 0.0-1.0, fraction of height for spectrum
    void setTunedFrequency(qint64 freq);
    void setFilterBandwidth(int bwHz);
    void setMode(const QString &mode);
    void setIfShift(int shift);          // IF shift position (0-99, 50=centered)
    void setCwPitch(int pitchHz);        // CW sidetone pitch in Hz
    void clear();

    // Display settings
    void setSpectrumColor(const QColor &color);
    void setGridEnabled(bool enabled);
    void setPeakHoldEnabled(bool enabled);
    void setRefLevel(int level);  // Dynamic REF level from K4 (#REF command)
    void setSpan(int spanHz);     // Dynamic span from K4 (#SPN command)
    int span() const { return m_spanHz; }
    void setNotchFilter(bool enabled, int pitchHz);  // Manual notch visualization

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

signals:
    void frequencyClicked(qint64 freq);
    void frequencyDragged(qint64 freq);
    void frequencyScrolled(int steps);  // Scroll wheel: positive = up, negative = down

private:
    void initColorLUT();
    void decompressBins(const QByteArray &bins, QVector<float> &out);
    void applySmoothing(const QVector<float> &input, QVector<float> &output);
    void upsampleSpectrum(const QVector<float> &input, QVector<float> &output, int targetWidth);
    void updateWaterfall(const QVector<float> &spectrum);
    void drawSpectrum(QPainter &painter, const QRect &rect);
    void drawWaterfall(QPainter &painter, const QRect &rect);
    void drawGrid(QPainter &painter, const QRect &rect);
    void drawFrequencyMarker(QPainter &painter, const QRect &rect);
    void drawFilterPassband(QPainter &painter, const QRect &rect);
    void drawNotchFilter(QPainter &painter, const QRect &rect);
    QRgb dbToColor(float db);
    float normalizeDb(float db);
    int freqToX(qint64 freq, int width);
    qint64 xToFreq(int x, int width);

    // Spectrum data
    QVector<float> m_currentSpectrum;  // Current smoothed spectrum in dB
    QVector<float> m_rawSpectrum;      // Raw unsmoothed data
    QVector<float> m_peakHold;         // Peak hold values

    // Waterfall data (circular buffer of spectrum lines)
    QVector<QVector<float>> m_waterfallHistory;  // Circular buffer of spectrum data
    int m_waterfallWriteIndex;                    // Next write position in circular buffer
    QImage m_waterfallImage;                      // Rendered image (recreated each frame)
    static const int WATERFALL_HISTORY = 256;

    // Color lookup table (256 entries)
    QVector<QRgb> m_colorLUT;

    // Frequency info
    qint64 m_centerFreq;
    qint32 m_sampleRate;               // Span width in Hz
    float m_noiseFloor;
    qint64 m_tunedFreq;
    int m_filterBw;
    QString m_mode;
    int m_ifShift = 50;                  // IF shift position (0-99, 50=centered)
    int m_cwPitch = 500;                 // CW sidetone pitch in Hz (250-950)

    // Display settings
    float m_minDb;
    float m_maxDb;
    float m_spectrumRatio;             // Fraction for spectrum (rest is waterfall)
    float m_smoothingAlpha;            // EMA smoothing factor
    float m_smoothedBaseline = 0.0f;   // Smoothed baseline for stable spectrum drawing
    QColor m_spectrumColor;
    bool m_gridEnabled;
    bool m_peakHoldEnabled;
    int m_refLevel;                    // REF level from K4 in dBm (default -110)
    int m_spanHz;                      // Span from #SPN CAT command (in Hz)

    // Peak hold decay
    QTimer *m_peakDecayTimer;
    static constexpr float PEAK_DECAY_RATE = 0.5f; // dB per tick

    // Waterfall marker visibility (hides after tuning stops)
    QTimer *m_waterfallMarkerTimer;
    bool m_showWaterfallMarker;

    // Notch filter visualization
    bool m_notchEnabled = false;
    int m_notchPitchHz = 0;  // Audio frequency offset (150-5000 Hz)
};

#endif // PANADAPTER_H
