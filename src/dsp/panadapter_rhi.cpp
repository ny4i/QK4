#include "panadapter_rhi.h"
#include <QFile>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QtMath>
#include <cmath>

// Transparent overlay widget for dBm scale labels
class DbmScaleOverlay : public QWidget {
public:
    DbmScaleOverlay(QWidget *parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_TranslucentBackground);
    }

    void setDbRange(float minDb, float maxDb) {
        m_minDb = minDb;
        m_maxDb = maxDb;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QFont scaleFont;
        scaleFont.setStyleHint(QFont::Monospace);
        scaleFont.setPointSize(8);
#ifdef Q_OS_MACOS
        scaleFont.setFamily("Menlo");
#elif defined(Q_OS_WIN)
        scaleFont.setFamily("Consolas");
#else
        scaleFont.setFamily("DejaVu Sans Mono");
#endif
        painter.setFont(scaleFont);
        painter.setPen(Qt::white);

        const int labelCount = 9;
        const float dbRange = m_maxDb - m_minDb;
        const int leftMargin = 4;
        const int h = height();

        QFontMetrics fm(scaleFont);
        int textHeight = fm.height();

        for (int i = 0; i < labelCount; ++i) {
            float dbValue = m_maxDb - (static_cast<float>(i) / 8.0f) * dbRange;
            int yPos = h * i / 8;

            QString label = QString("%1 dBm").arg(static_cast<int>(dbValue));

            // Vertically center on grid line
            int textY = yPos + textHeight / 3;

            // Adjust top and bottom labels to stay in bounds
            if (i == 0)
                textY = textHeight - 2;
            if (i == labelCount - 1)
                textY = h - 4;

            painter.drawText(leftMargin, textY, label);
        }
    }

private:
    float m_minDb = -138.0f;
    float m_maxDb = -58.0f;
};

PanadapterRhiWidget::PanadapterRhiWidget(QWidget *parent) : QRhiWidget(parent) {
    setMinimumHeight(200);
    setMouseTracking(true);

    // Debug: Check what APIs are available
    qDebug() << "=== PanadapterRhiWidget Constructor ===";
    qDebug() << "Platform:" << QSysInfo::prettyProductName();
    qDebug() << "Qt version:" << QT_VERSION_STR;

    // Force Metal API on macOS
#ifdef Q_OS_MACOS
    qDebug() << "Requesting Metal API...";
    setApi(QRhiWidget::Api::Metal);
    qDebug() << "API after setApi:" << static_cast<int>(api());
#endif

    // Initialize color LUTs
    initColorLUT();    // Waterfall LUT
    initSpectrumLUT(); // Spectrum LUT (for BlueAmplitude style)

    // Note: Waterfall data buffer is allocated in initialize() after devicePixelRatio is known

    // Peak hold decay timer
    m_peakDecayTimer = new QTimer(this);
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
    m_peakDecayTimer->start(50);

    // Waterfall marker timer
    m_waterfallMarkerTimer = new QTimer(this);
    m_waterfallMarkerTimer->setSingleShot(true);
    connect(m_waterfallMarkerTimer, &QTimer::timeout, this, [this]() {
        m_showWaterfallMarker = false;
        update();
    });

    // Create dBm scale overlay (child widget)
    m_dbmScaleOverlay = new DbmScaleOverlay(this);
    m_dbmScaleOverlay->setDbRange(m_minDb, m_maxDb);
    m_dbmScaleOverlay->show();
}

PanadapterRhiWidget::~PanadapterRhiWidget() {
    // QRhi resources are automatically cleaned up
}

void PanadapterRhiWidget::resizeEvent(QResizeEvent *event) {
    QRhiWidget::resizeEvent(event);
    updateDbmScaleOverlay();
}

void PanadapterRhiWidget::updateDbmScaleOverlay() {
    if (!m_dbmScaleOverlay)
        return;

    // Position overlay to cover spectrum area only (top portion)
    const int h = height();
    const int spectrumHeight = static_cast<int>(h * m_spectrumRatio);

    // Overlay covers left side of spectrum area
    m_dbmScaleOverlay->setGeometry(0, 0, 70, spectrumHeight);
    m_dbmScaleOverlay->setDbRange(m_minDb, m_maxDb);
    m_dbmScaleOverlay->raise(); // Ensure it's on top
}

void PanadapterRhiWidget::initColorLUT() {
    // Create 256-entry RGBA color LUT for WATERFALL (unchanged)
    // 8-stage color progression: Black -> Dark Blue -> Royal Blue -> Cyan -> Green -> Yellow -> Red
    m_colorLUT.resize(256 * 4);

    for (int i = 0; i < 256; ++i) {
        float value = i / 255.0f;
        int r, g, b;

        if (value < 0.10f) {
            r = 0;
            g = 0;
            b = 0;
        } else if (value < 0.25f) {
            float t = (value - 0.10f) / 0.15f;
            r = 0;
            g = 0;
            b = static_cast<int>(t * 51);
        } else if (value < 0.40f) {
            float t = (value - 0.25f) / 0.15f;
            r = 0;
            g = 0;
            b = static_cast<int>(51 + t * 102);
        } else if (value < 0.55f) {
            float t = (value - 0.40f) / 0.15f;
            r = 0;
            g = static_cast<int>(t * 128);
            b = static_cast<int>(153 + t * 102);
        } else if (value < 0.70f) {
            float t = (value - 0.55f) / 0.15f;
            r = 0;
            g = static_cast<int>(128 + t * 127);
            b = static_cast<int>(255 * (1.0f - t));
        } else if (value < 0.85f) {
            float t = (value - 0.70f) / 0.15f;
            r = static_cast<int>(t * 255);
            g = 255;
            b = 0;
        } else {
            float t = (value - 0.85f) / 0.15f;
            r = 255;
            g = static_cast<int>(255 * (1.0f - t));
            b = 0;
        }

        m_colorLUT[i * 4 + 0] = static_cast<quint8>(qBound(0, r, 255));
        m_colorLUT[i * 4 + 1] = static_cast<quint8>(qBound(0, g, 255));
        m_colorLUT[i * 4 + 2] = static_cast<quint8>(qBound(0, b, 255));
        m_colorLUT[i * 4 + 3] = 255;
    }
}

void PanadapterRhiWidget::initSpectrumLUT() {
    // Create 256-entry RGBA color LUT for SPECTRUM (BlueAmplitude style)
    // 8-stage: Royal Blue -> Cyan -> Green -> Yellow -> Orange -> Red -> White
    // Noise floor starts at royal blue (more visible color earlier)
    m_spectrumLUT.resize(256 * 4);

    for (int i = 0; i < 256; ++i) {
        float value = i / 255.0f;
        int r, g, b;

        if (value < 0.08f) {
            // Royal Blue (visible noise floor) - start brighter
            float t = value / 0.08f;
            r = 0;
            g = 0;
            b = static_cast<int>(80 + t * 100); // 80-180: royal blue range
        } else if (value < 0.20f) {
            // Royal Blue -> Cyan
            float t = (value - 0.08f) / 0.12f;
            r = 0;
            g = static_cast<int>(t * 200);
            b = static_cast<int>(180 + t * 75); // 180-255
        } else if (value < 0.35f) {
            // Cyan -> Green
            float t = (value - 0.20f) / 0.15f;
            r = 0;
            g = static_cast<int>(200 + t * 55); // 200-255
            b = static_cast<int>(255 * (1.0f - t));
        } else if (value < 0.52f) {
            // Green -> Yellow
            float t = (value - 0.35f) / 0.17f;
            r = static_cast<int>(t * 255);
            g = 255;
            b = 0;
        } else if (value < 0.70f) {
            // Yellow -> Orange -> Red
            float t = (value - 0.52f) / 0.18f;
            r = 255;
            g = static_cast<int>(255 * (1.0f - t));
            b = 0;
        } else {
            // Red -> White (strongest signals)
            float t = (value - 0.70f) / 0.30f;
            r = 255;
            g = static_cast<int>(t * 255);
            b = static_cast<int>(t * 255);
        }

        m_spectrumLUT[i * 4 + 0] = static_cast<quint8>(qBound(0, r, 255));
        m_spectrumLUT[i * 4 + 1] = static_cast<quint8>(qBound(0, g, 255));
        m_spectrumLUT[i * 4 + 2] = static_cast<quint8>(qBound(0, b, 255));
        m_spectrumLUT[i * 4 + 3] = 255;
    }
}

void PanadapterRhiWidget::initialize(QRhiCommandBuffer *cb) {
    qDebug() << "=== PanadapterRhiWidget::initialize() ===";
    qDebug() << "Already initialized:" << m_rhiInitialized;
    qDebug() << "Widget visible:" << isVisible();
    qDebug() << "Widget size:" << size();
    qDebug() << "API requested:" << static_cast<int>(api());

    if (m_rhiInitialized)
        return;

    m_rhi = rhi();
    qDebug() << "rhi() returned:" << m_rhi;
    if (!m_rhi) {
        qWarning() << "!!! QRhi is NULL - Metal backend failed to initialize !!!";
        qWarning() << "Check if Metal framework is available and GPU supports Metal";
        return;
    }
    qDebug() << "QRhi backend name:" << m_rhi->backendName();
    qDebug() << "QRhi driver info:" << m_rhi->driverInfo().deviceName;

    // Use fixed texture sizes - GPU bilinear filtering handles scaling to display size
    m_textureWidth = BASE_TEXTURE_WIDTH;
    m_waterfallHistory = BASE_WATERFALL_HISTORY;
    qDebug() << "Texture width:" << m_textureWidth;
    qDebug() << "Waterfall history:" << m_waterfallHistory;

    // Allocate waterfall data buffer
    m_waterfallData.resize(m_textureWidth * m_waterfallHistory);
    m_waterfallData.fill(0);

    // Load shaders from compiled .qsb resources
    auto loadShader = [](const QString &path) -> QShader {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly))
            return QShader::fromSerialized(f.readAll());
        qWarning() << "Failed to load shader:" << path;
        return QShader();
    };

    m_spectrumBlueVert = loadShader(":/shaders/src/dsp/shaders/spectrum_blue.vert.qsb");
    m_spectrumBlueFrag = loadShader(":/shaders/src/dsp/shaders/spectrum_blue.frag.qsb");
    m_spectrumBlueAmpFrag = loadShader(":/shaders/src/dsp/shaders/spectrum_blue_amp.frag.qsb");
    m_waterfallVert = loadShader(":/shaders/src/dsp/shaders/waterfall.vert.qsb");
    m_waterfallFrag = loadShader(":/shaders/src/dsp/shaders/waterfall.frag.qsb");
    m_overlayVert = loadShader(":/shaders/src/dsp/shaders/overlay.vert.qsb");
    m_overlayFrag = loadShader(":/shaders/src/dsp/shaders/overlay.frag.qsb");

    // Create waterfall texture (single channel for dB values)
    m_waterfallTexture.reset(m_rhi->newTexture(QRhiTexture::R8, QSize(m_textureWidth, m_waterfallHistory), 1,
                                               QRhiTexture::UsedAsTransferSource));
    m_waterfallTexture->create();

    // Create color LUT texture (256x1 RGBA)
    m_colorLutTexture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, QSize(256, 1)));
    m_colorLutTexture->create();

    // Create spectrum data texture (1D texture for fragment shader spectrum)
    m_spectrumDataTexture.reset(m_rhi->newTexture(QRhiTexture::R32F, QSize(m_textureWidth, 1)));
    m_spectrumDataTexture->create();

    // Create spectrum color LUT texture (256x1 RGBA) - for BlueAmplitude style
    m_spectrumColorLutTexture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, QSize(256, 1)));
    m_spectrumColorLutTexture->create();

    // Upload color LUT data (separate LUTs for waterfall and spectrum)
    QRhiResourceUpdateBatch *rub = m_rhi->nextResourceUpdateBatch();
    // Upload waterfall color LUT
    QRhiTextureSubresourceUploadDescription waterfallLutUpload(m_colorLUT.constData(), m_colorLUT.size());
    rub->uploadTexture(m_colorLutTexture.get(), QRhiTextureUploadEntry(0, 0, waterfallLutUpload));
    // Upload spectrum color LUT (for BlueAmplitude style)
    QRhiTextureSubresourceUploadDescription spectrumLutUpload(m_spectrumLUT.constData(), m_spectrumLUT.size());
    rub->uploadTexture(m_spectrumColorLutTexture.get(), QRhiTextureUploadEntry(0, 0, spectrumLutUpload));

    // Upload initial zeroed waterfall data (prevents uninitialized texture garbage)
    QRhiTextureSubresourceUploadDescription waterfallUpload(m_waterfallData.constData(), m_waterfallData.size());
    rub->uploadTexture(m_waterfallTexture.get(), QRhiTextureUploadEntry(0, 0, waterfallUpload));

    // Create sampler
    m_sampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                      QRhiSampler::ClampToEdge, QRhiSampler::Repeat));
    m_sampler->create();

    // Waterfall quad (static)
    float tMax = static_cast<float>(m_waterfallHistory - 1) / m_waterfallHistory;
    float waterfallQuad[] = {
        // position (x, y), texcoord (s, t)
        -1.0f, -1.0f, 0.0f, 0.0f, // bottom-left
        1.0f,  -1.0f, 1.0f, 0.0f, // bottom-right
        1.0f,  1.0f,  1.0f, tMax, // top-right
        -1.0f, -1.0f, 0.0f, 0.0f, // bottom-left
        1.0f,  1.0f,  1.0f, tMax, // top-right
        -1.0f, 1.0f,  0.0f, tMax  // top-left
    };
    m_waterfallVbo.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(waterfallQuad)));
    m_waterfallVbo->create();
    rub->uploadStaticBuffer(m_waterfallVbo.get(), waterfallQuad);

    m_overlayVbo.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, 4096 * 2 * sizeof(float)));
    m_overlayVbo->create();

    // Create uniform buffers
    m_waterfallUniformBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 16));
    m_waterfallUniformBuffer->create();

    m_overlayUniformBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 32));
    m_overlayUniformBuffer->create();

    // Fullscreen quad (shared by all fragment-shader spectrum styles)
    // Position (x, y) + texCoord (s, t) - covers normalized -1 to 1 range
    float fullscreenQuad[] = {
        // position (x, y), texcoord (s, t)
        -1.0f, -1.0f, 0.0f, 1.0f, // bottom-left (texCoord y=1 = bottom)
        1.0f,  -1.0f, 1.0f, 1.0f, // bottom-right
        1.0f,  1.0f,  1.0f, 0.0f, // top-right (texCoord y=0 = top)
        -1.0f, -1.0f, 0.0f, 1.0f, // bottom-left
        1.0f,  1.0f,  1.0f, 0.0f, // top-right
        -1.0f, 1.0f,  0.0f, 0.0f  // top-left
    };
    m_fullscreenQuadVbo.reset(
        m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(fullscreenQuad)));
    m_fullscreenQuadVbo->create();
    rub->uploadStaticBuffer(m_fullscreenQuadVbo.get(), fullscreenQuad);

    // Blue spectrum style uniform buffer: 80 bytes (std140 layout)
    // fillBaseColor(16) + fillPeakColor(16) + glowColor(16) + params(16) + viewport(16)
    m_spectrumBlueUniformBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 80));
    m_spectrumBlueUniformBuffer->create();

    // Separate buffers for passband to avoid GPU buffer conflicts
    m_passbandVbo.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, 256 * sizeof(float)));
    m_passbandVbo->create();

    m_passbandUniformBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 32));
    m_passbandUniformBuffer->create();

    // Separate buffers for marker to avoid conflicts with passband
    m_markerVbo.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, 64 * sizeof(float)));
    m_markerVbo->create();

    m_markerUniformBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 32));
    m_markerUniformBuffer->create();

    // Separate buffers for notch to avoid conflicts with grid (which shares overlay buffers)
    m_notchVbo.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, 64 * sizeof(float)));
    m_notchVbo->create();

    m_notchUniformBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 32));
    m_notchUniformBuffer->create();

    cb->resourceUpdate(rub);

    m_rhiInitialized = true;
}

void PanadapterRhiWidget::createPipelines() {
    if (m_pipelinesCreated)
        return;

    if (!m_spectrumBlueVert.isValid() || !m_spectrumBlueFrag.isValid())
        return;

    m_rpDesc = renderTarget()->renderPassDescriptor();

    // Spectrum blue pipeline (blue gradient with cyan glow) - Blue style
    {
        m_spectrumBlueSrb.reset(m_rhi->newShaderResourceBindings());
        m_spectrumBlueSrb->setBindings(
            {QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::FragmentStage,
                                                      m_spectrumBlueUniformBuffer.get()),
             QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                       m_spectrumDataTexture.get(), m_sampler.get())});
        m_spectrumBlueSrb->create();

        m_spectrumBluePipeline.reset(m_rhi->newGraphicsPipeline());
        m_spectrumBluePipeline->setShaderStages(
            {{QRhiShaderStage::Vertex, m_spectrumBlueVert}, {QRhiShaderStage::Fragment, m_spectrumBlueFrag}});

        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({{4 * sizeof(float)}});                         // position(2) + texcoord(2)
        inputLayout.setAttributes({{0, 0, QRhiVertexInputAttribute::Float2, 0}, // position
                                   {0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float)}}); // texcoord
        m_spectrumBluePipeline->setVertexInputLayout(inputLayout);
        m_spectrumBluePipeline->setTopology(QRhiGraphicsPipeline::Triangles);
        m_spectrumBluePipeline->setShaderResourceBindings(m_spectrumBlueSrb.get());
        m_spectrumBluePipeline->setRenderPassDescriptor(m_rpDesc);

        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
        blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        m_spectrumBluePipeline->setTargetBlends({blend});

        m_spectrumBluePipeline->create();
    }

    // Spectrum blue amplitude pipeline (amplitude-based brightness) - BlueAmplitude style
    // Shares uniform buffer with Blue style, uses different fragment shader
    {
        m_spectrumBlueAmpSrb.reset(m_rhi->newShaderResourceBindings());
        m_spectrumBlueAmpSrb->setBindings(
            {QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::FragmentStage,
                                                      m_spectrumBlueUniformBuffer.get()),
             QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                       m_spectrumDataTexture.get(), m_sampler.get()),
             QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage,
                                                       m_spectrumColorLutTexture.get(), m_sampler.get())});
        m_spectrumBlueAmpSrb->create();

        m_spectrumBlueAmpPipeline.reset(m_rhi->newGraphicsPipeline());
        m_spectrumBlueAmpPipeline->setShaderStages(
            {{QRhiShaderStage::Vertex, m_spectrumBlueVert}, {QRhiShaderStage::Fragment, m_spectrumBlueAmpFrag}});

        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({{4 * sizeof(float)}});                         // position(2) + texcoord(2)
        inputLayout.setAttributes({{0, 0, QRhiVertexInputAttribute::Float2, 0}, // position
                                   {0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float)}}); // texcoord
        m_spectrumBlueAmpPipeline->setVertexInputLayout(inputLayout);
        m_spectrumBlueAmpPipeline->setTopology(QRhiGraphicsPipeline::Triangles);
        m_spectrumBlueAmpPipeline->setShaderResourceBindings(m_spectrumBlueAmpSrb.get());
        m_spectrumBlueAmpPipeline->setRenderPassDescriptor(m_rpDesc);

        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
        blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        m_spectrumBlueAmpPipeline->setTargetBlends({blend});

        m_spectrumBlueAmpPipeline->create();
    }

    // Waterfall pipeline
    {
        m_waterfallSrb.reset(m_rhi->newShaderResourceBindings());
        m_waterfallSrb->setBindings(
            {QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage,
                                                      m_waterfallUniformBuffer.get()),
             QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                       m_waterfallTexture.get(), m_sampler.get()),
             QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage,
                                                       m_colorLutTexture.get(), m_sampler.get())});
        m_waterfallSrb->create();

        m_waterfallPipeline.reset(m_rhi->newGraphicsPipeline());
        m_waterfallPipeline->setShaderStages(
            {{QRhiShaderStage::Vertex, m_waterfallVert}, {QRhiShaderStage::Fragment, m_waterfallFrag}});

        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({{4 * sizeof(float)}});                         // position(2) + texcoord(2)
        inputLayout.setAttributes({{0, 0, QRhiVertexInputAttribute::Float2, 0}, // position
                                   {0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float)}}); // texcoord
        m_waterfallPipeline->setVertexInputLayout(inputLayout);
        m_waterfallPipeline->setTopology(QRhiGraphicsPipeline::Triangles);
        m_waterfallPipeline->setShaderResourceBindings(m_waterfallSrb.get());
        m_waterfallPipeline->setRenderPassDescriptor(m_rpDesc);
        m_waterfallPipeline->create();
    }

    // Overlay pipeline (lines)
    {
        m_overlaySrb.reset(m_rhi->newShaderResourceBindings());
        m_overlaySrb->setBindings({QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            m_overlayUniformBuffer.get())});
        m_overlaySrb->create();

        // Separate SRB for passband to avoid buffer conflicts
        m_passbandSrb.reset(m_rhi->newShaderResourceBindings());
        m_passbandSrb->setBindings({QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            m_passbandUniformBuffer.get())});
        m_passbandSrb->create();

        // Separate SRB for marker to avoid buffer conflicts with passband
        m_markerSrb.reset(m_rhi->newShaderResourceBindings());
        m_markerSrb->setBindings({QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            m_markerUniformBuffer.get())});
        m_markerSrb->create();

        m_notchSrb.reset(m_rhi->newShaderResourceBindings());
        m_notchSrb->setBindings({QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            m_notchUniformBuffer.get())});
        m_notchSrb->create();

        m_overlayLinePipeline.reset(m_rhi->newGraphicsPipeline());
        m_overlayLinePipeline->setShaderStages(
            {{QRhiShaderStage::Vertex, m_overlayVert}, {QRhiShaderStage::Fragment, m_overlayFrag}});

        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({{2 * sizeof(float)}});
        inputLayout.setAttributes({{0, 0, QRhiVertexInputAttribute::Float2, 0}});
        m_overlayLinePipeline->setVertexInputLayout(inputLayout);
        m_overlayLinePipeline->setTopology(QRhiGraphicsPipeline::Lines);
        m_overlayLinePipeline->setShaderResourceBindings(m_overlaySrb.get());
        m_overlayLinePipeline->setRenderPassDescriptor(m_rpDesc);

        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
        blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        m_overlayLinePipeline->setTargetBlends({blend});

        m_overlayLinePipeline->create();

        // Triangle version for filled shapes
        m_overlayTrianglePipeline.reset(m_rhi->newGraphicsPipeline());
        m_overlayTrianglePipeline->setShaderStages(
            {{QRhiShaderStage::Vertex, m_overlayVert}, {QRhiShaderStage::Fragment, m_overlayFrag}});
        m_overlayTrianglePipeline->setVertexInputLayout(inputLayout);
        m_overlayTrianglePipeline->setTopology(QRhiGraphicsPipeline::Triangles);
        m_overlayTrianglePipeline->setShaderResourceBindings(m_overlaySrb.get());
        m_overlayTrianglePipeline->setRenderPassDescriptor(m_rpDesc);
        m_overlayTrianglePipeline->setTargetBlends({blend});
        m_overlayTrianglePipeline->create();
    }

    m_pipelinesCreated = true;
}

void PanadapterRhiWidget::render(QRhiCommandBuffer *cb) {
    // Always clear to black even if not initialized (prevents red/garbage showing)
    if (!m_rhiInitialized) {
        cb->beginPass(renderTarget(), Qt::black, {1.0f, 0}, nullptr);
        cb->endPass();
        return;
    }

    // Create pipelines on first render (need render pass descriptor)
    if (!m_pipelinesCreated) {
        createPipelines();
        if (!m_pipelinesCreated) {
            cb->beginPass(renderTarget(), Qt::black, {1.0f, 0}, nullptr);
            cb->endPass();
            return;
        }
    }

    const QSize outputSize = renderTarget()->pixelSize();
    const float w = outputSize.width();
    const float h = outputSize.height();
    const float spectrumHeight = h * m_spectrumRatio;
    const float waterfallHeight = h - spectrumHeight;

    QRhiResourceUpdateBatch *rub = m_rhi->nextResourceUpdateBatch();

    // Update waterfall texture if needed
    if (m_waterfallNeedsUpdate && !m_currentSpectrum.isEmpty()) {
        updateWaterfallData();
        QRhiTextureSubresourceUploadDescription rowUpload(
            m_waterfallData.constData() + m_waterfallWriteRow * m_textureWidth, m_textureWidth);
        rowUpload.setDestinationTopLeft(QPoint(0, m_waterfallWriteRow));
        rowUpload.setSourceSize(QSize(m_textureWidth, 1));
        rub->uploadTexture(m_waterfallTexture.get(), QRhiTextureUploadEntry(0, 0, rowUpload));
        m_waterfallWriteRow = (m_waterfallWriteRow + 1) % m_waterfallHistory;
        m_waterfallNeedsUpdate = false;
    }

    // Update waterfall uniform buffer
    float scrollOffset = static_cast<float>(m_waterfallWriteRow) / m_waterfallHistory;
    struct {
        float scrollOffset;
        float padding[3];
    } waterfallUniforms = {scrollOffset, {0, 0, 0}};
    rub->updateDynamicBuffer(m_waterfallUniformBuffer.get(), 0, sizeof(waterfallUniforms), &waterfallUniforms);

    // Calculate smoothed baseline for spectrum normalization
    if (!m_currentSpectrum.isEmpty()) {
        float frameMinNormalized = 1.0f;
        for (int i = 0; i < m_currentSpectrum.size(); ++i) {
            float normalized = normalizeDb(m_currentSpectrum[i]);
            if (normalized < frameMinNormalized)
                frameMinNormalized = normalized;
        }
        const float baselineAlpha = 0.05f;
        if (m_smoothedBaseline < 0.001f)
            m_smoothedBaseline = frameMinNormalized;
        else
            m_smoothedBaseline = baselineAlpha * frameMinNormalized + (1.0f - baselineAlpha) * m_smoothedBaseline;

        // Upload spectrum data to 1D texture (must be done before beginPass)
        QVector<float> normalizedSpectrum(m_textureWidth);
        for (int i = 0; i < m_textureWidth; ++i) {
            float srcPos = static_cast<float>(i) / (m_textureWidth - 1) * (m_currentSpectrum.size() - 1);
            int idx = qBound(0, qRound(srcPos), m_currentSpectrum.size() - 1);
            float normalized = normalizeDb(m_currentSpectrum[idx]);
            float adjusted = qMax(0.0f, normalized - m_smoothedBaseline);
            normalizedSpectrum[i] = adjusted * 0.95f;
        }

        QRhiTextureSubresourceUploadDescription specDataUpload(normalizedSpectrum.constData(),
                                                               normalizedSpectrum.size() * sizeof(float));
        specDataUpload.setSourceSize(QSize(m_textureWidth, 1));
        rub->uploadTexture(m_spectrumDataTexture.get(), QRhiTextureUploadEntry(0, 0, specDataUpload));

        // Update blue spectrum uniform buffer (80 bytes, std140 layout)
        struct {
            float fillBaseColor[4]; // offset 0: dark navy
            float fillPeakColor[4]; // offset 16: electric blue
            float glowColor[4];     // offset 32: cyan glow
            float glowIntensity;    // offset 48
            float glowWidth;        // offset 52
            float spectrumHeightPx; // offset 56
            float padding1;         // offset 60
            float viewportSize[2];  // offset 64
            float padding2[2];      // offset 72
        } specBlueUniforms = {
            {0.0f, 0.08f, 0.16f, 0.85f}, // fillBaseColor: dark navy
            {0.0f, 0.63f, 1.0f, 0.85f},  // fillPeakColor: electric blue
            {0.0f, 0.83f, 1.0f, 1.0f},   // glowColor: cyan
            0.8f,                        // glowIntensity
            0.04f,                       // glowWidth
            spectrumHeight,              // spectrumHeight in pixels
            0.0f,                        // padding1
            {w, h},                      // viewportSize
            {0.0f, 0.0f}                 // padding2
        };
        rub->updateDynamicBuffer(m_spectrumBlueUniformBuffer.get(), 0, sizeof(specBlueUniforms), &specBlueUniforms);
    }

    cb->resourceUpdate(rub);

    // Begin render pass
    cb->beginPass(renderTarget(), QColor::fromRgbF(0.08f, 0.08f, 0.08f, 1.0f), {1.0f, 0}, nullptr);

    // Draw waterfall (bottom portion)
    if (m_waterfallPipeline) {
        cb->setViewport({0, 0, w, waterfallHeight});
        cb->setGraphicsPipeline(m_waterfallPipeline.get());
        cb->setShaderResources(m_waterfallSrb.get());
        const QRhiCommandBuffer::VertexInput waterfallVbufBinding(m_waterfallVbo.get(), 0);
        cb->setVertexInput(0, 1, &waterfallVbufBinding);
        cb->draw(6);
    }

    // Draw grid BEHIND spectrum (in spectrum area)
    if (m_gridEnabled && m_overlayLinePipeline) {
        cb->setViewport({0, waterfallHeight, w, spectrumHeight});

        QVector<float> gridVerts;

        // Horizontal lines (dB scale) - 8 divisions in spectrum area
        for (int i = 1; i < 8; ++i) {
            float y = spectrumHeight * i / 8.0f;
            gridVerts << 0.0f << y << w << y;
        }

        // Vertical lines (frequency) - 10 divisions in spectrum area
        for (int i = 1; i < 10; ++i) {
            float x = w * i / 10.0f;
            gridVerts << x << 0.0f << x << spectrumHeight;
        }

        QRhiResourceUpdateBatch *gridRub = m_rhi->nextResourceUpdateBatch();
        gridRub->updateDynamicBuffer(m_overlayVbo.get(), 0, gridVerts.size() * sizeof(float), gridVerts.constData());

        struct {
            float viewportWidth;
            float viewportHeight;
            float pad0, pad1;
            float r, g, b, a;
        } gridUniforms = {w,
                          spectrumHeight,
                          0,
                          0,
                          static_cast<float>(m_gridColor.redF()),
                          static_cast<float>(m_gridColor.greenF()),
                          static_cast<float>(m_gridColor.blueF()),
                          static_cast<float>(m_gridColor.alphaF())};
        gridRub->updateDynamicBuffer(m_overlayUniformBuffer.get(), 0, sizeof(gridUniforms), &gridUniforms);

        cb->resourceUpdate(gridRub);
        cb->setGraphicsPipeline(m_overlayLinePipeline.get());
        cb->setShaderResources(m_overlaySrb.get());
        const QRhiCommandBuffer::VertexInput gridVbufBinding(m_overlayVbo.get(), 0);
        cb->setVertexInput(0, 1, &gridVbufBinding);
        cb->draw(gridVerts.size() / 2);
    }

    // Draw spectrum fill ON TOP of grid (shader-based fullscreen quad)
    if (!m_currentSpectrum.isEmpty()) {
        cb->setViewport({0, waterfallHeight, w, spectrumHeight});

        if (m_spectrumStyle == SpectrumStyle::BlueAmplitude && m_spectrumBlueAmpPipeline) {
            // BlueAmplitude style: LUT-based colors with amplitude brightness
            cb->setGraphicsPipeline(m_spectrumBlueAmpPipeline.get());
            cb->setShaderResources(m_spectrumBlueAmpSrb.get());
        } else if (m_spectrumBluePipeline) {
            // Blue style: gradient with cyan glow (Y-position based)
            cb->setGraphicsPipeline(m_spectrumBluePipeline.get());
            cb->setShaderResources(m_spectrumBlueSrb.get());
        }

        const QRhiCommandBuffer::VertexInput quadVbufBinding(m_fullscreenQuadVbo.get(), 0);
        cb->setVertexInput(0, 1, &quadVbufBinding);
        cb->draw(6); // Fullscreen quad (2 triangles)
    }

    // Draw overlays (full viewport for grid, markers, passband)
    cb->setViewport({0, 0, w, h});

    if (m_overlayLinePipeline && m_overlayTrianglePipeline) {
        // Helper lambda to draw filled quad
        auto drawFilledQuad = [&](float x1, float y1, float x2, float y2, const QColor &color) {
            QVector<float> quadVerts = {x1, y1, x2, y1, x2, y2, x1, y1, x2, y2, x1, y2};

            QRhiResourceUpdateBatch *rub2 = m_rhi->nextResourceUpdateBatch();
            rub2->updateDynamicBuffer(m_overlayVbo.get(), 0, quadVerts.size() * sizeof(float), quadVerts.constData());

            struct {
                float viewportWidth;
                float viewportHeight;
                float pad0, pad1; // Matches shader's vec2 padding (std140 layout)
                float r, g, b, a; // Matches shader's vec4 color at offset 16
            } overlayUniforms = {w,
                                 h,
                                 0,
                                 0,
                                 static_cast<float>(color.redF()),
                                 static_cast<float>(color.greenF()),
                                 static_cast<float>(color.blueF()),
                                 static_cast<float>(color.alphaF())};
            rub2->updateDynamicBuffer(m_overlayUniformBuffer.get(), 0, sizeof(overlayUniforms), &overlayUniforms);

            cb->resourceUpdate(rub2);
            cb->setGraphicsPipeline(m_overlayTrianglePipeline.get());
            cb->setShaderResources(m_overlaySrb.get());
            const QRhiCommandBuffer::VertexInput overlayVbufBinding(m_overlayVbo.get(), 0);
            cb->setVertexInput(0, 1, &overlayVbufBinding);
            cb->draw(6);
        };

        // Helper lambda to draw lines
        auto drawLines = [&](const QVector<float> &lineVerts, const QColor &color) {
            if (lineVerts.isEmpty())
                return;
            QRhiResourceUpdateBatch *rub2 = m_rhi->nextResourceUpdateBatch();
            rub2->updateDynamicBuffer(m_overlayVbo.get(), 0, lineVerts.size() * sizeof(float), lineVerts.constData());

            struct {
                float viewportWidth;
                float viewportHeight;
                float pad0, pad1; // Matches shader's vec2 padding (std140 layout)
                float r, g, b, a; // Matches shader's vec4 color at offset 16
            } overlayUniforms = {w,
                                 h,
                                 0,
                                 0,
                                 static_cast<float>(color.redF()),
                                 static_cast<float>(color.greenF()),
                                 static_cast<float>(color.blueF()),
                                 static_cast<float>(color.alphaF())};
            rub2->updateDynamicBuffer(m_overlayUniformBuffer.get(), 0, sizeof(overlayUniforms), &overlayUniforms);

            cb->resourceUpdate(rub2);
            cb->setGraphicsPipeline(m_overlayLinePipeline.get());
            cb->setShaderResources(m_overlaySrb.get());
            const QRhiCommandBuffer::VertexInput overlayVbufBinding(m_overlayVbo.get(), 0);
            cb->setVertexInput(0, 1, &overlayVbufBinding);
            cb->draw(lineVerts.size() / 2);
        };

        // Grid is now drawn BEFORE spectrum fill (see above)

        // Draw passband overlay (uses separate buffers to avoid GPU conflicts)
        if (m_cursorVisible && m_filterBw > 0 && m_tunedFreq > 0) {
            // Calculate passband edges based on mode
            qint64 lowFreq, highFreq;

            // K4 IF shift is reported in decahertz (10 Hz units)
            // This is the passband center offset from the dial frequency
            // USB with shift=150 means passband centered 1500 Hz above dial
            // CW with shift=50 means passband centered at 500 Hz pitch
            int shiftOffsetHz = m_ifShift * 10;

            if (m_mode == "LSB") {
                // LSB: passband is below dial, shift indicates center offset (negative)
                qint64 center = m_tunedFreq - shiftOffsetHz;
                lowFreq = center - m_filterBw / 2;
                highFreq = center + m_filterBw / 2;
            } else if (m_mode == "USB" || m_mode == "DATA" || m_mode == "DATA-R") {
                // USB: passband is above dial, shift indicates center offset
                qint64 center = m_tunedFreq + shiftOffsetHz;
                lowFreq = center - m_filterBw / 2;
                highFreq = center + m_filterBw / 2;
            } else if (m_mode == "CW" || m_mode == "CW-R") {
                // CW: shift already includes pitch offset from K4
                int pitchOffset = (m_mode == "CW") ? m_cwPitch : -m_cwPitch;
                qint64 center = m_tunedFreq + pitchOffset;
                lowFreq = center - m_filterBw / 2;
                highFreq = center + m_filterBw / 2;
            } else {
                // AM/FM - symmetric around tuned freq + shift
                qint64 center = m_tunedFreq + shiftOffsetHz;
                lowFreq = center - m_filterBw / 2;
                highFreq = center + m_filterBw / 2;
            }

            // Convert to pixel coordinates
            float x1 = freqToNormalized(lowFreq) * w;
            float x2 = freqToNormalized(highFreq) * w;

            // Clamp to visible area
            x1 = qBound(0.0f, x1, w);
            x2 = qBound(0.0f, x2, w);

            if (x2 > x1) {
                // Draw passband quad - use dedicated VBO, uniform buffer, and SRB
                // Use spectrumHeight not h - passband should only appear in spectrum area, not waterfall
                QVector<float> quadVerts = {
                    x1, 0, x2, 0, x2, spectrumHeight, x1, 0, x2, spectrumHeight, x1, spectrumHeight};

                QRhiResourceUpdateBatch *pbRub = m_rhi->nextResourceUpdateBatch();
                pbRub->updateDynamicBuffer(m_passbandVbo.get(), 0, quadVerts.size() * sizeof(float),
                                           quadVerts.constData());

                struct {
                    float viewportWidth;
                    float viewportHeight;
                    float pad0, pad1;
                    float r, g, b, a;
                } pbUniforms = {w,
                                h,
                                0,
                                0,
                                static_cast<float>(m_passbandColor.redF()),
                                static_cast<float>(m_passbandColor.greenF()),
                                static_cast<float>(m_passbandColor.blueF()),
                                static_cast<float>(m_passbandColor.alphaF())};
                pbRub->updateDynamicBuffer(m_passbandUniformBuffer.get(), 0, sizeof(pbUniforms), &pbUniforms);

                cb->resourceUpdate(pbRub);
                cb->setGraphicsPipeline(m_overlayTrianglePipeline.get());
                cb->setShaderResources(m_passbandSrb.get());
                const QRhiCommandBuffer::VertexInput pbVbufBinding(m_passbandVbo.get(), 0);
                cb->setVertexInput(0, 1, &pbVbufBinding);
                cb->draw(6);
            }

            // Draw frequency marker - use dedicated VBO, uniform buffer, and SRB
            // Use spectrumHeight not h - marker should only appear in spectrum area, not waterfall
            // For CW modes: marker at passband center (dial + pitch offset)
            // For SSB/other: marker at dial frequency (passband shifts around it)
            qint64 markerFreq = m_tunedFreq;
            if (m_mode == "CW") {
                // CW marker at passband center (pitch offset from dial)
                markerFreq = m_tunedFreq + m_cwPitch;
            } else if (m_mode == "CW-R") {
                // CW-R marker at passband center (pitch offset below dial)
                markerFreq = m_tunedFreq - m_cwPitch;
            }
            // For USB/LSB/AM/FM: marker stays at dial frequency
            float markerX = freqToNormalized(markerFreq) * w;
            if (markerX >= 0 && markerX <= w) {
                // Draw as filled rectangle (2px wide) instead of line for robust Metal rendering
                float markerWidth = 2.0f;
                QVector<float> markerVerts = {markerX,
                                              0.0f,
                                              markerX + markerWidth,
                                              0.0f,
                                              markerX + markerWidth,
                                              spectrumHeight,
                                              markerX,
                                              0.0f,
                                              markerX + markerWidth,
                                              spectrumHeight,
                                              markerX,
                                              spectrumHeight};

                QRhiResourceUpdateBatch *mkRub = m_rhi->nextResourceUpdateBatch();
                mkRub->updateDynamicBuffer(m_markerVbo.get(), 0, markerVerts.size() * sizeof(float),
                                           markerVerts.constData());

                struct {
                    float viewportWidth;
                    float viewportHeight;
                    float pad0, pad1;
                    float r, g, b, a;
                } mkUniforms = {w,
                                h,
                                0,
                                0,
                                static_cast<float>(m_frequencyMarkerColor.redF()),
                                static_cast<float>(m_frequencyMarkerColor.greenF()),
                                static_cast<float>(m_frequencyMarkerColor.blueF()),
                                static_cast<float>(m_frequencyMarkerColor.alphaF())};
                mkRub->updateDynamicBuffer(m_markerUniformBuffer.get(), 0, sizeof(mkUniforms), &mkUniforms);

                cb->resourceUpdate(mkRub);
                cb->setGraphicsPipeline(m_overlayTrianglePipeline.get());
                cb->setShaderResources(m_markerSrb.get());
                const QRhiCommandBuffer::VertexInput mkVbufBinding(m_markerVbo.get(), 0);
                cb->setVertexInput(0, 1, &mkVbufBinding);
                cb->draw(6);
            }

            // Draw notch filter marker (red line) - uses dedicated notch buffers
            if (m_notchEnabled && m_notchPitchHz > 0) {
                qint64 notchFreq;
                if (m_mode == "LSB") {
                    notchFreq = m_tunedFreq - m_notchPitchHz;
                } else {
                    notchFreq = m_tunedFreq + m_notchPitchHz;
                }

                float notchX = freqToNormalized(notchFreq) * w;
                bool inBounds = (notchX >= 0 && notchX <= w);

                if (inBounds) {
                    // Draw as filled rectangle (2px wide) instead of line for robust rendering
                    float notchWidth = 2.0f;
                    QVector<float> notchVerts = {notchX,
                                                 0.0f,
                                                 notchX + notchWidth,
                                                 0.0f,
                                                 notchX + notchWidth,
                                                 spectrumHeight,
                                                 notchX,
                                                 0.0f,
                                                 notchX + notchWidth,
                                                 spectrumHeight,
                                                 notchX,
                                                 spectrumHeight};

                    QRhiResourceUpdateBatch *notchRub = m_rhi->nextResourceUpdateBatch();
                    notchRub->updateDynamicBuffer(m_notchVbo.get(), 0, notchVerts.size() * sizeof(float),
                                                  notchVerts.constData());

                    struct {
                        float viewportWidth;
                        float viewportHeight;
                        float pad0, pad1;
                        float r, g, b, a;
                    } notchUniforms = {w,
                                       h,
                                       0,
                                       0,
                                       static_cast<float>(m_notchColor.redF()),
                                       static_cast<float>(m_notchColor.greenF()),
                                       static_cast<float>(m_notchColor.blueF()),
                                       static_cast<float>(m_notchColor.alphaF())};
                    notchRub->updateDynamicBuffer(m_notchUniformBuffer.get(), 0, sizeof(notchUniforms), &notchUniforms);

                    cb->resourceUpdate(notchRub);
                    cb->setGraphicsPipeline(m_overlayTrianglePipeline.get()); // Use triangles not lines
                    cb->setShaderResources(m_notchSrb.get());
                    const QRhiCommandBuffer::VertexInput notchVbufBinding(m_notchVbo.get(), 0);
                    cb->setVertexInput(0, 1, &notchVbufBinding);
                    cb->draw(6); // 2 triangles = 6 vertices
                }
            }
        }
    }

    cb->endPass();
}

void PanadapterRhiWidget::updateSpectrum(const QByteArray &bins, qint64 centerFreq, qint32 sampleRate,
                                         float noiseFloor) {
    m_centerFreq = centerFreq;
    m_sampleRate = sampleRate;
    m_noiseFloor = noiseFloor;

    // K4 tier span = sampleRate * 1000 Hz
    qint32 tierSpanHz = sampleRate * 1000;
    int totalBins = bins.size();

    // Extract center bins if tier span > commanded span
    QByteArray binsToUse;
    if (tierSpanHz > m_spanHz && totalBins > 100 && m_spanHz > 0) {
        int requestedBins = (static_cast<qint64>(m_spanHz) * totalBins) / tierSpanHz;
        requestedBins = qBound(50, requestedBins, totalBins);
        int centerStart = (totalBins - requestedBins + 1) / 2; // Round up for symmetric extraction
        binsToUse = bins.mid(centerStart, requestedBins);
    } else {
        binsToUse = bins;
    }

    // Decompress bins to dB values
    decompressBins(binsToUse, m_rawSpectrum);

    // Apply exponential smoothing for gradual decay (attack fast, decay slow)
    constexpr float attackAlpha = 0.85f; // Fast attack (new peaks appear quickly)
    constexpr float decayAlpha = 0.45f;  // Moderate decay for crisp waterfall

    if (m_currentSpectrum.size() != m_rawSpectrum.size()) {
        m_currentSpectrum = m_rawSpectrum;
    } else {
        for (int i = 0; i < m_rawSpectrum.size(); ++i) {
            float alpha = (m_rawSpectrum[i] > m_currentSpectrum[i]) ? attackAlpha : decayAlpha;
            m_currentSpectrum[i] = alpha * m_rawSpectrum[i] + (1.0f - alpha) * m_currentSpectrum[i];
        }
    }

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

    m_waterfallNeedsUpdate = true;
    update();
}

void PanadapterRhiWidget::updateMiniSpectrum(const QByteArray &bins) {
    m_rawSpectrum.resize(bins.size());
    for (int i = 0; i < bins.size(); ++i) {
        m_rawSpectrum[i] = static_cast<quint8>(bins[i]) * 10.0f - 160.0f;
    }

    // Apply exponential smoothing for gradual decay (attack fast, decay slow)
    constexpr float attackAlpha = 0.85f; // Fast attack
    constexpr float decayAlpha = 0.38f;  // Slower decay (visible glow effect)

    if (m_currentSpectrum.size() != m_rawSpectrum.size()) {
        m_currentSpectrum = m_rawSpectrum;
    } else {
        for (int i = 0; i < m_rawSpectrum.size(); ++i) {
            float alpha = (m_rawSpectrum[i] > m_currentSpectrum[i]) ? attackAlpha : decayAlpha;
            m_currentSpectrum[i] = alpha * m_rawSpectrum[i] + (1.0f - alpha) * m_currentSpectrum[i];
        }
    }

    m_waterfallNeedsUpdate = true;
    update();
}

void PanadapterRhiWidget::decompressBins(const QByteArray &bins, QVector<float> &out) {
    // K4 spectrum bins: dBm = byte - 146
    // Calibrated by comparing peak signals with K4 display
    out.resize(bins.size());
    for (int i = 0; i < bins.size(); ++i) {
        out[i] = static_cast<quint8>(bins[i]) - 146.0f;
    }
}

void PanadapterRhiWidget::updateWaterfallData() {
    if (m_currentSpectrum.isEmpty())
        return;

    // Resample spectrum to texture width
    int row = m_waterfallWriteRow;
    for (int i = 0; i < m_textureWidth; ++i) {
        float srcPos = static_cast<float>(i) / (m_textureWidth - 1) * (m_currentSpectrum.size() - 1);
        int idx = qBound(0, qRound(srcPos), m_currentSpectrum.size() - 1);
        float normalized = normalizeDb(m_currentSpectrum[idx]);
        m_waterfallData[row * m_textureWidth + i] =
            static_cast<quint8>(qBound(0, static_cast<int>(normalized * 255), 255));
    }
}

float PanadapterRhiWidget::normalizeDb(float db) {
    return qBound(0.0f, (db - m_minDb) / (m_maxDb - m_minDb), 1.0f);
}

float PanadapterRhiWidget::freqToNormalized(qint64 freq) {
    // Map frequency to normalized range [0.0, 1.0] where:
    // - 0.0 = left edge (startFreq)
    // - 1.0 = right edge (startFreq + spanHz)
    //
    // IMPORTANT: In CW mode, the K4 centers the spectrum on (dial + cwPitch), not the dial frequency.
    // This is because the IF center is offset by the CW sidetone pitch.
    qint64 effectiveCenter = m_centerFreq;
    if (m_mode == "CW") {
        effectiveCenter = m_centerFreq + m_cwPitch;
    } else if (m_mode == "CW-R") {
        effectiveCenter = m_centerFreq - m_cwPitch;
    }
    qint64 startFreq = effectiveCenter - m_spanHz / 2;
    return static_cast<float>(freq - startFreq) / static_cast<float>(m_spanHz);
}

qint64 PanadapterRhiWidget::xToFreq(int x, int w) {
    // Map pixel position to frequency for click-to-tune
    // Use floating point for precision
    //
    // NOTE: Do NOT apply CW pitch offset here. The user clicks on a signal at a certain
    // visual position. That signal's frequency is what we want to tune to.
    // The spectrum display already shows frequencies correctly; we just need to map
    // the click position back to frequency using the centerFreq from the K4.
    if (w <= 0)
        return m_centerFreq;
    qint64 startFreq = m_centerFreq - m_spanHz / 2;
    double normalized = static_cast<double>(x) / static_cast<double>(w);
    return startFreq + static_cast<qint64>(normalized * m_spanHz);
}

QColor PanadapterRhiWidget::interpolateColor(const QColor &a, const QColor &b, float t) {
    t = qBound(0.0f, t, 1.0f);
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t, a.greenF() + (b.greenF() - a.greenF()) * t,
                            a.blueF() + (b.blueF() - a.blueF()) * t, a.alphaF() + (b.alphaF() - a.alphaF()) * t);
}

QColor PanadapterRhiWidget::spectrumGradientColor(float t) {
    // 5-stop gradient: visible dark lime → lime green → bright lime → light lime → white
    // Creates a lime green spectrum fill with visible base color
    struct GradientStop {
        float pos;
        int r, g, b, a;
    };
    static const GradientStop stops[] = {
        {0.00f, 20, 60, 20, 128},    // Visible dark lime (50% alpha)
        {0.15f, 40, 120, 30, 180},   // Translucent lime green
        {0.50f, 80, 200, 60, 220},   // Bright lime green
        {0.75f, 160, 255, 120, 245}, // Light lime with yellow hint
        {1.00f, 255, 255, 255, 255}  // Pure white peak
    };

    t = qBound(0.0f, t, 1.0f);

    // Find surrounding stops and interpolate
    for (int i = 0; i < 4; ++i) {
        if (t <= stops[i + 1].pos) {
            float localT = (t - stops[i].pos) / (stops[i + 1].pos - stops[i].pos);
            QColor c1(stops[i].r, stops[i].g, stops[i].b, stops[i].a);
            QColor c2(stops[i + 1].r, stops[i + 1].g, stops[i + 1].b, stops[i + 1].a);
            return interpolateColor(c1, c2, localT);
        }
    }
    return QColor(255, 255, 255, 255); // Clamp to white
}

// Configuration setters
void PanadapterRhiWidget::setDbRange(float minDb, float maxDb) {
    m_minDb = minDb;
    m_maxDb = maxDb;
    update();
}

void PanadapterRhiWidget::setSpectrumRatio(float ratio) {
    m_spectrumRatio = qBound(0.1f, ratio, 0.9f);
    updateDbmScaleOverlay(); // Resize dBm scale to match new spectrum area
    update();
}

void PanadapterRhiWidget::setWaterfallHeight(int percent) {
    // Waterfall height percentage: 50% means 50% waterfall, 50% spectrum
    // Spectrum ratio = (100 - waterfallHeight) / 100
    float ratio = (100.0f - qBound(10, percent, 90)) / 100.0f;
    m_spectrumRatio = qBound(0.1f, ratio, 0.9f);
    updateDbmScaleOverlay(); // Resize dBm scale to match new spectrum area
    update();
}

void PanadapterRhiWidget::setTunedFrequency(qint64 freq) {
    if (m_tunedFreq != freq) {
        m_tunedFreq = freq;
        m_showWaterfallMarker = true;
        m_waterfallMarkerTimer->start(500);
        update();
    }
}

void PanadapterRhiWidget::setFilterBandwidth(int bwHz) {
    m_filterBw = bwHz;
    update();
}

void PanadapterRhiWidget::setMode(const QString &mode) {
    m_mode = mode;
    update();
}

void PanadapterRhiWidget::setIfShift(int shift) {
    if (m_ifShift != shift) {
        m_ifShift = shift;
        update();
    }
}

void PanadapterRhiWidget::setCwPitch(int pitchHz) {
    if (m_cwPitch != pitchHz) {
        m_cwPitch = pitchHz;
        update();
    }
}

void PanadapterRhiWidget::clear() {
    m_currentSpectrum.clear();
    m_rawSpectrum.clear();
    m_peakHold.clear();
    m_waterfallWriteRow = 0;
    m_waterfallData.fill(0);
    update();
}

void PanadapterRhiWidget::setGridEnabled(bool enabled) {
    m_gridEnabled = enabled;
    update();
}

void PanadapterRhiWidget::setPeakHoldEnabled(bool enabled) {
    m_peakHoldEnabled = enabled;
    if (!enabled)
        m_peakHold.clear();
    update();
}

void PanadapterRhiWidget::setRefLevel(int level) {
    if (m_refLevel != level) {
        m_refLevel = level;
        updateDbRangeFromRefAndScale();
        update();
    }
}

void PanadapterRhiWidget::setScale(int scale) {
    // Scale range: 10-150 (per K4 documentation)
    // Higher values = more compressed display (signals appear weaker, wider dB range)
    // Lower values = more expanded display (signals appear stronger, narrower dB range)
    if (m_scale != scale && scale >= 10 && scale <= 150) {
        m_scale = scale;
        updateDbRangeFromRefAndScale();
        update();
    }
}

void PanadapterRhiWidget::updateDbRangeFromRefAndScale() {
    // RefLevel is the bottom reference, scale is the dB range upward
    // Display shows from refLevel to (refLevel + scale)
    m_minDb = static_cast<float>(m_refLevel);
    m_maxDb = static_cast<float>(m_refLevel) + static_cast<float>(m_scale);

    updateDbmScaleOverlay();
}

void PanadapterRhiWidget::setSpan(int spanHz) {
    if (m_spanHz != spanHz && spanHz > 0) {
        m_spanHz = spanHz;
        update();
    }
}

void PanadapterRhiWidget::setNotchFilter(bool enabled, int pitchHz) {
    if (m_notchEnabled != enabled || m_notchPitchHz != pitchHz) {
        m_notchEnabled = enabled;
        m_notchPitchHz = pitchHz;
        update();
    }
}

void PanadapterRhiWidget::setCursorVisible(bool visible) {
    if (m_cursorVisible != visible) {
        m_cursorVisible = visible;
        update();
    }
}

// Color setters
void PanadapterRhiWidget::setSpectrumBaseColor(const QColor &color) {
    m_spectrumBaseColor = color;
    update();
}

void PanadapterRhiWidget::setSpectrumPeakColor(const QColor &color) {
    m_spectrumPeakColor = color;
    update();
}

void PanadapterRhiWidget::setSpectrumLineColor(const QColor &color) {
    m_spectrumLineColor = color;
    update();
}

void PanadapterRhiWidget::setGridColor(const QColor &color) {
    m_gridColor = color;
    update();
}

void PanadapterRhiWidget::setPeakHoldColor(const QColor &color) {
    m_peakHoldColor = color;
    update();
}

void PanadapterRhiWidget::setPassbandColor(const QColor &color) {
    m_passbandColor = color;
    update();
}

void PanadapterRhiWidget::setFrequencyMarkerColor(const QColor &color) {
    m_frequencyMarkerColor = color;
    update();
}

void PanadapterRhiWidget::setNotchColor(const QColor &color) {
    m_notchColor = color;
    update();
}

void PanadapterRhiWidget::setBackgroundGradient(const QColor &center, const QColor &edge) {
    m_bgCenterColor = center;
    m_bgEdgeColor = edge;
    update();
}

void PanadapterRhiWidget::setSpectrumStyle(SpectrumStyle style) {
    if (m_spectrumStyle != style) {
        m_spectrumStyle = style;
        update();
    }
}

// Mouse events
void PanadapterRhiWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        qint64 freq = xToFreq(event->pos().x(), width());
        emit frequencyClicked(freq);
    }
}

void PanadapterRhiWidget::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        qint64 freq = xToFreq(event->pos().x(), width());
        emit frequencyDragged(freq);
    }
}

void PanadapterRhiWidget::wheelEvent(QWheelEvent *event) {
    int degrees = event->angleDelta().y() / 8;
    int steps = degrees / 15;

    if (steps != 0) {
        emit frequencyScrolled(steps);
    }
    event->accept();
}
