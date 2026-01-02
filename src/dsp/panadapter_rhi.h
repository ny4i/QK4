#ifndef PANADAPTER_RHI_H
#define PANADAPTER_RHI_H

#include <QRhiWidget>
#include <rhi/qrhi.h>
#include <QColor>
#include <QTimer>
#include <QVector>
#include <memory>

// Modern GPU-accelerated panadapter using Qt RHI
// Supports Metal (macOS), DirectX (Windows), Vulkan (Linux)
class PanadapterRhiWidget : public QRhiWidget {
    Q_OBJECT

public:
    explicit PanadapterRhiWidget(QWidget *parent = nullptr);
    ~PanadapterRhiWidget();

    // Update spectrum data from K4 PAN packet
    void updateSpectrum(const QByteArray &bins, qint64 centerFreq, qint32 sampleRate, float noiseFloor);

    // Update from MiniPAN packet (simpler format)
    void updateMiniSpectrum(const QByteArray &bins);

    // Configuration
    void setDbRange(float minDb, float maxDb);
    void setSpectrumRatio(float ratio);
    void setTunedFrequency(qint64 freq);
    void setFilterBandwidth(int bwHz);
    void setMode(const QString &mode);
    void setIfShift(int shift);
    void setCwPitch(int pitchHz);
    void clear();

    // Display settings
    void setGridEnabled(bool enabled);
    void setPeakHoldEnabled(bool enabled);
    void setRefLevel(int level);
    void setSpan(int spanHz);
    int span() const { return m_spanHz; }
    void setNotchFilter(bool enabled, int pitchHz);
    void setCursorVisible(bool visible);

    // Color configuration (NEW: all colors configurable)
    void setSpectrumBaseColor(const QColor &color);
    void setSpectrumPeakColor(const QColor &color);
    void setSpectrumLineColor(const QColor &color);
    void setGridColor(const QColor &color);
    void setPeakHoldColor(const QColor &color);
    void setPassbandColor(const QColor &color);
    void setFrequencyMarkerColor(const QColor &color);
    void setNotchColor(const QColor &color);
    void setBackgroundGradient(const QColor &center, const QColor &edge);

signals:
    void frequencyClicked(qint64 freq);
    void frequencyDragged(qint64 freq);
    void frequencyScrolled(int steps);

protected:
    // QRhiWidget overrides
    void initialize(QRhiCommandBuffer *cb) override;
    void render(QRhiCommandBuffer *cb) override;

    // Input events
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    // Initialization
    void initColorLUT();
    void createPipelines();

    // Data processing
    void decompressBins(const QByteArray &bins, QVector<float> &out);
    void updateWaterfallData();

    // Coordinate helpers
    float normalizeDb(float db);
    float freqToNormalized(qint64 freq);
    qint64 xToFreq(int x, int w);
    QColor interpolateColor(const QColor &a, const QColor &b, float t);
    QColor spectrumGradientColor(float t); // 5-stop teal-to-white gradient

    // RHI resources
    QRhi *m_rhi = nullptr;
    std::unique_ptr<QRhiBuffer> m_spectrumVbo;
    std::unique_ptr<QRhiBuffer> m_spectrumUniformBuffer;
    std::unique_ptr<QRhiBuffer> m_waterfallVbo;
    std::unique_ptr<QRhiBuffer> m_waterfallUniformBuffer;
    std::unique_ptr<QRhiBuffer> m_overlayVbo;
    std::unique_ptr<QRhiBuffer> m_overlayUniformBuffer;
    std::unique_ptr<QRhiBuffer> m_passbandVbo;
    std::unique_ptr<QRhiBuffer> m_passbandUniformBuffer;
    std::unique_ptr<QRhiBuffer> m_markerVbo;
    std::unique_ptr<QRhiBuffer> m_markerUniformBuffer;
    std::unique_ptr<QRhiTexture> m_waterfallTexture;
    std::unique_ptr<QRhiTexture> m_colorLutTexture;
    std::unique_ptr<QRhiTexture> m_spectrumDataTexture; // 1D texture for spectrum values
    std::unique_ptr<QRhiSampler> m_sampler;
    std::unique_ptr<QRhiGraphicsPipeline> m_spectrumPipeline;
    std::unique_ptr<QRhiGraphicsPipeline> m_spectrumFillPipeline; // Fragment shader spectrum
    std::unique_ptr<QRhiGraphicsPipeline> m_waterfallPipeline;
    std::unique_ptr<QRhiGraphicsPipeline> m_overlayLinePipeline;
    std::unique_ptr<QRhiGraphicsPipeline> m_overlayTrianglePipeline;
    std::unique_ptr<QRhiShaderResourceBindings> m_spectrumSrb;
    std::unique_ptr<QRhiShaderResourceBindings> m_spectrumFillSrb;
    std::unique_ptr<QRhiBuffer> m_spectrumFillVbo;
    std::unique_ptr<QRhiBuffer> m_spectrumFillUniformBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> m_waterfallSrb;
    std::unique_ptr<QRhiShaderResourceBindings> m_overlaySrb;
    std::unique_ptr<QRhiShaderResourceBindings> m_passbandSrb;
    std::unique_ptr<QRhiShaderResourceBindings> m_markerSrb;
    QRhiRenderPassDescriptor *m_rpDesc = nullptr;

    bool m_rhiInitialized = false;
    bool m_pipelinesCreated = false;
    bool m_firstFrameRendered = false;

    // Shader stages (loaded from .qsb files)
    QShader m_spectrumVert;
    QShader m_spectrumFrag;
    QShader m_spectrumFillVert;
    QShader m_spectrumFillFrag;
    QShader m_waterfallVert;
    QShader m_waterfallFrag;
    QShader m_overlayVert;
    QShader m_overlayFrag;

    // Spectrum data
    QVector<float> m_currentSpectrum;
    QVector<float> m_rawSpectrum;
    QVector<float> m_peakHold;

    // Waterfall data
    static constexpr int WATERFALL_HISTORY = 256;
    static constexpr int TEXTURE_WIDTH = 2048;
    int m_waterfallWriteRow = 0;
    QVector<quint8> m_waterfallData;
    bool m_waterfallNeedsUpdate = false;

    // Color LUT (256 RGBA entries)
    QVector<quint8> m_colorLUT;

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
    float m_smoothedBaseline = 0.0f;
    bool m_gridEnabled = true;
    bool m_peakHoldEnabled = true;
    int m_refLevel = -110;
    int m_spanHz = 10000;
    bool m_notchEnabled = false;
    int m_notchPitchHz = 0;
    bool m_cursorVisible = true;

    // Colors (all configurable)
    // Spectrum gradient uses spectrumGradientColor() for 5-stop lime-to-white
    QColor m_spectrumBaseColor{20, 60, 20, 128};    // Visible dark lime (gradient stop 0.0)
    QColor m_spectrumPeakColor{255, 255, 255, 255}; // Pure white peak (gradient stop 1.0)
    QColor m_spectrumLineColor{50, 255, 50};        // Lime green line on top
    QColor m_peakTrailColor{60, 140, 60};           // Dim lime green for peak trail
    QColor m_gridColor{102, 102, 102, 77};          // Gray with 30% alpha
    QColor m_peakHoldColor{255, 255, 255, 102};     // White with 40% alpha
    QColor m_passbandColor{0, 128, 255, 64};        // Blue with 25% alpha
    QColor m_frequencyMarkerColor{0, 80, 200};      // Dark blue
    QColor m_notchColor{255, 0, 0};                 // Red
    QColor m_bgCenterColor{56, 56, 56};             // Lighter gray at center
    QColor m_bgEdgeColor{20, 20, 20};               // Darker at edges

    // Peak hold decay
    QTimer *m_peakDecayTimer = nullptr;
    static constexpr float PEAK_DECAY_RATE = 0.5f;

    // Waterfall marker
    QTimer *m_waterfallMarkerTimer = nullptr;
    bool m_showWaterfallMarker = false;
};

#endif // PANADAPTER_RHI_H
