#ifndef MINIPANWIDGET_H
#define MINIPANWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QVector>
#include <QColor>

// Compact Mini-Pan widget for VFO area
// Shows simplified spectrum + waterfall in a small form factor
// Fixed ±1kHz span around VFO frequency
// GPU-accelerated via OpenGL for smooth rendering
class MiniPanWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit MiniPanWidget(QWidget *parent = nullptr);
    ~MiniPanWidget();

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
    // OpenGL overrides
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    // Input events
    void mousePressEvent(QMouseEvent *event) override;

private:
    // OpenGL initialization helpers
    void initShaders();
    void initTextures();
    void initBuffers();
    void initColorLUT();

    // OpenGL rendering
    void drawWaterfallGL();
    void drawSpectrumGL(int spectrumHeight);
    void drawOverlaysGL();

    // Data processing
    float normalizeDb(float db);

    // Helper to determine bandwidth from mode
    int bandwidthForMode(const QString &mode) const;

    // OpenGL resources
    QOpenGLShaderProgram *m_waterfallShader = nullptr;
    QOpenGLShaderProgram *m_spectrumShader = nullptr;
    QOpenGLShaderProgram *m_overlayShader = nullptr;

    GLuint m_waterfallTexture = 0;
    GLuint m_colorLutTexture = 0;
    int m_waterfallWriteRow = 0;

    QOpenGLBuffer m_spectrumVBO;
    QOpenGLBuffer m_waterfallVBO;
    QOpenGLBuffer m_overlayVBO;
    QOpenGLVertexArrayObject m_vao;

    bool m_glInitialized = false;
    static const int TEXTURE_WIDTH = 512;
    static const int WATERFALL_HISTORY = 100;

    // Color lookup table (256 RGBA entries for GPU)
    QVector<GLubyte> m_colorLUT;

    // Spectrum data
    QVector<float> m_spectrum;         // Current spectrum in dB
    QVector<float> m_smoothedSpectrum; // Smoothed for display

    // Spectrum line color (customizable for VFO A vs B)
    QColor m_spectrumColor = QColor("#FFB000"); // Default amber (VFO A)

    // Passband color (customizable for VFO A vs B)
    QColor m_passbandColor = QColor(0, 128, 255, 64); // Default blue (VFO A)

    // Display settings - range for Mini-Pan relative data
    float m_minDb = -1.0f;
    float m_maxDb = 4.0f;
    float m_smoothingAlpha = 0.3f;
    float m_smoothedBaseline = 0.0f;
    float m_heightBoost = 1.5f;
    float m_spectrumRatio = 0.40f; // 40% spectrum, 60% waterfall

    // Notch filter visualization
    bool m_notchEnabled = false;
    int m_notchPitchHz = 0;
    QString m_mode = "USB";
    int m_bandwidthHz = 10000; // Mode-dependent span: CW=3kHz, Voice/Data=10kHz

    // Filter passband visualization
    int m_filterBw = 2400;
    int m_ifShift = 50;
    int m_cwPitch = 600;
};

#endif // MINIPANWIDGET_H
