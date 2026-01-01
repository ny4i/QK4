#ifndef PANADAPTER_H
#define PANADAPTER_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QVector>
#include <QColor>
#include <QTimer>

// Combined panadapter (spectrum) and waterfall display widget
// GPU-accelerated via OpenGL for smooth rendering at any resolution
class PanadapterWidget : public QOpenGLWidget, protected QOpenGLFunctions {
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
    void setIfShift(int shift);   // IF shift position (0-99, 50=centered)
    void setCwPitch(int pitchHz); // CW sidetone pitch in Hz
    void clear();

    // Display settings
    void setSpectrumColor(const QColor &color);
    void setPassbandColor(const QColor &color);        // Filter passband overlay color
    void setFrequencyMarkerColor(const QColor &color); // Tuned frequency line color
    void setGridEnabled(bool enabled);
    void setPeakHoldEnabled(bool enabled);
    void setRefLevel(int level); // Dynamic REF level from K4 (#REF command)
    void setSpan(int spanHz);    // Dynamic span from K4 (#SPN command)
    int span() const { return m_spanHz; }
    void setNotchFilter(bool enabled, int pitchHz); // Manual notch visualization
    void setCursorVisible(bool visible);            // VFO cursor/passband visibility

protected:
    // OpenGL overrides
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    // Input events
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

signals:
    void frequencyClicked(qint64 freq);
    void frequencyDragged(qint64 freq);
    void frequencyScrolled(int steps); // Scroll wheel: positive = up, negative = down

private:
    // OpenGL initialization helpers
    void initShaders();
    void initTextures();
    void initBuffers();
    void initColorLUT();

    // OpenGL rendering
    void drawWaterfall();
    void drawSpectrum(int spectrumHeight);
    void drawOverlays();

    // Data processing
    void decompressBins(const QByteArray &bins, QVector<float> &out);
    void applySmoothing(const QVector<float> &input, QVector<float> &output);
    void updateWaterfallTexture(const QVector<float> &spectrum);

    // Coordinate helpers
    float normalizeDb(float db);
    float freqToNormalized(qint64 freq);
    qint64 xToFreq(int x, int w);

    // OpenGL resources
    QOpenGLShaderProgram *m_waterfallShader = nullptr;
    QOpenGLShaderProgram *m_spectrumShader = nullptr;
    QOpenGLShaderProgram *m_overlayShader = nullptr;

    GLuint m_waterfallTexture = 0; // 256-row waterfall data texture
    GLuint m_colorLutTexture = 0;  // 256-entry color lookup texture
    int m_waterfallWriteRow = 0;   // Current row in circular texture

    QOpenGLBuffer m_spectrumVBO;  // Spectrum line vertices
    QOpenGLBuffer m_waterfallVBO; // Waterfall quad vertices
    QOpenGLBuffer m_overlayVBO;   // Grid/markers vertices
    QOpenGLVertexArrayObject m_vao;

    bool m_glInitialized = false;
    int m_textureWidth = 2048; // Power of 2 for texture width
    static const int WATERFALL_HISTORY = 256;

    // Color lookup table (256 RGBA entries for GPU)
    QVector<GLubyte> m_colorLUT;

    // Spectrum data
    QVector<float> m_currentSpectrum; // Current smoothed spectrum in dB
    QVector<float> m_rawSpectrum;     // Raw unsmoothed data
    QVector<float> m_peakHold;        // Peak hold values

    // Frequency info
    qint64 m_centerFreq = 0;
    qint32 m_sampleRate = 192000;
    float m_noiseFloor = -130.0f;
    qint64 m_tunedFreq = 0;
    int m_filterBw = 2400;
    QString m_mode = "USB";
    int m_ifShift = 50;
    int m_cwPitch = 500;

    // Display settings
    float m_minDb = -138.0f;
    float m_maxDb = -58.0f;
    float m_spectrumRatio = 0.30f;
    float m_smoothingAlpha = 0.80f;
    float m_smoothedBaseline = 0.0f;
    QColor m_spectrumColor{0xFF, 0xB0, 0x00}; // Amber
    QColor m_passbandColor{0, 128, 255};
    QColor m_frequencyMarkerColor{0, 80, 200};
    bool m_gridEnabled = true;
    bool m_peakHoldEnabled = true;
    int m_refLevel = -110;
    int m_spanHz = 10000;

    // Peak hold decay
    QTimer *m_peakDecayTimer = nullptr;
    static constexpr float PEAK_DECAY_RATE = 0.5f;

    // Waterfall marker visibility
    QTimer *m_waterfallMarkerTimer = nullptr;
    bool m_showWaterfallMarker = false;

    // Notch filter visualization
    bool m_notchEnabled = false;
    int m_notchPitchHz = 0;

    // VFO cursor/passband visibility
    bool m_cursorVisible = true;
};

#endif // PANADAPTER_H
