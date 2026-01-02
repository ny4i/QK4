#include "panadapter_rhi.h"
#include <QFile>
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>
#include <cmath>

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

    // Initialize color LUT
    initColorLUT();

    // Allocate waterfall data buffer
    m_waterfallData.resize(TEXTURE_WIDTH * WATERFALL_HISTORY);
    m_waterfallData.fill(0);

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
}

PanadapterRhiWidget::~PanadapterRhiWidget() {
    // QRhi resources are automatically cleaned up
}

void PanadapterRhiWidget::initColorLUT() {
    // Create 256-entry RGBA color LUT for waterfall
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

    // Load shaders from compiled .qsb resources
    auto loadShader = [](const QString &path) -> QShader {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly))
            return QShader::fromSerialized(f.readAll());
        qWarning() << "Failed to load shader:" << path;
        return QShader();
    };

    m_spectrumVert = loadShader(":/shaders/src/dsp/shaders/spectrum.vert.qsb");
    m_spectrumFrag = loadShader(":/shaders/src/dsp/shaders/spectrum.frag.qsb");
    m_spectrumFillVert = loadShader(":/shaders/src/dsp/shaders/spectrum_fill.vert.qsb");
    m_spectrumFillFrag = loadShader(":/shaders/src/dsp/shaders/spectrum_fill.frag.qsb");
    m_waterfallVert = loadShader(":/shaders/src/dsp/shaders/waterfall.vert.qsb");
    m_waterfallFrag = loadShader(":/shaders/src/dsp/shaders/waterfall.frag.qsb");
    m_overlayVert = loadShader(":/shaders/src/dsp/shaders/overlay.vert.qsb");
    m_overlayFrag = loadShader(":/shaders/src/dsp/shaders/overlay.frag.qsb");

    // Create waterfall texture (single channel for dB values)
    m_waterfallTexture.reset(m_rhi->newTexture(QRhiTexture::R8, QSize(TEXTURE_WIDTH, WATERFALL_HISTORY), 1,
                                               QRhiTexture::UsedAsTransferSource));
    m_waterfallTexture->create();

    // Create color LUT texture (256x1 RGBA)
    m_colorLutTexture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, QSize(256, 1)));
    m_colorLutTexture->create();

    // Create spectrum data texture (1D texture for fragment shader spectrum)
    m_spectrumDataTexture.reset(m_rhi->newTexture(QRhiTexture::R32F, QSize(TEXTURE_WIDTH, 1)));
    m_spectrumDataTexture->create();

    // Upload color LUT data
    QRhiResourceUpdateBatch *rub = m_rhi->nextResourceUpdateBatch();
    QRhiTextureSubresourceUploadDescription lutUpload(m_colorLUT.constData(), m_colorLUT.size());
    rub->uploadTexture(m_colorLutTexture.get(), QRhiTextureUploadEntry(0, 0, lutUpload));

    // Upload initial zeroed waterfall data (prevents uninitialized texture garbage)
    QRhiTextureSubresourceUploadDescription waterfallUpload(m_waterfallData.constData(), m_waterfallData.size());
    rub->uploadTexture(m_waterfallTexture.get(), QRhiTextureUploadEntry(0, 0, waterfallUpload));

    // Create sampler
    m_sampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                      QRhiSampler::ClampToEdge, QRhiSampler::Repeat));
    m_sampler->create();

    // Create vertex buffers (dynamic)
    m_spectrumVbo.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, 16384 * 6 * sizeof(float)));
    m_spectrumVbo->create();

    // Waterfall quad (static)
    float tMax = static_cast<float>(WATERFALL_HISTORY - 1) / WATERFALL_HISTORY;
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
    m_spectrumUniformBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 16));
    m_spectrumUniformBuffer->create();

    m_waterfallUniformBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 16));
    m_waterfallUniformBuffer->create();

    m_overlayUniformBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 32));
    m_overlayUniformBuffer->create();

    // Spectrum fill quad (fullscreen quad for fragment shader spectrum)
    // Position (x, y) + texCoord (s, t) - covers normalized -1 to 1 range
    float spectrumFillQuad[] = {
        // position (x, y), texcoord (s, t)
        -1.0f, -1.0f, 0.0f, 1.0f, // bottom-left (texCoord y=1 = bottom)
        1.0f,  -1.0f, 1.0f, 1.0f, // bottom-right
        1.0f,  1.0f,  1.0f, 0.0f, // top-right (texCoord y=0 = top)
        -1.0f, -1.0f, 0.0f, 1.0f, // bottom-left
        1.0f,  1.0f,  1.0f, 0.0f, // top-right
        -1.0f, 1.0f,  0.0f, 0.0f  // top-left
    };
    m_spectrumFillVbo.reset(
        m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(spectrumFillQuad)));
    m_spectrumFillVbo->create();
    rub->uploadStaticBuffer(m_spectrumFillVbo.get(), spectrumFillQuad);

    // Spectrum fill uniform buffer: spectrumHeight, frostHeight, pad, pad, peakColor(4), baseColor(4)
    m_spectrumFillUniformBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 48));
    m_spectrumFillUniformBuffer->create();

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

    cb->resourceUpdate(rub);

    m_rhiInitialized = true;
}

void PanadapterRhiWidget::createPipelines() {
    if (m_pipelinesCreated)
        return;

    if (!m_spectrumVert.isValid() || !m_spectrumFrag.isValid())
        return;

    m_rpDesc = renderTarget()->renderPassDescriptor();

    // Spectrum pipeline (triangle strip with per-vertex color)
    {
        m_spectrumSrb.reset(m_rhi->newShaderResourceBindings());
        m_spectrumSrb->setBindings({QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage,
                                                                             m_spectrumUniformBuffer.get())});
        m_spectrumSrb->create();

        m_spectrumPipeline.reset(m_rhi->newGraphicsPipeline());
        m_spectrumPipeline->setShaderStages(
            {{QRhiShaderStage::Vertex, m_spectrumVert}, {QRhiShaderStage::Fragment, m_spectrumFrag}});

        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({{6 * sizeof(float)}});                         // position(2) + color(4)
        inputLayout.setAttributes({{0, 0, QRhiVertexInputAttribute::Float2, 0}, // position
                                   {0, 1, QRhiVertexInputAttribute::Float4, 2 * sizeof(float)}}); // color
        m_spectrumPipeline->setVertexInputLayout(inputLayout);
        m_spectrumPipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
        m_spectrumPipeline->setShaderResourceBindings(m_spectrumSrb.get());
        m_spectrumPipeline->setRenderPassDescriptor(m_rpDesc);

        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
        blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        m_spectrumPipeline->setTargetBlends({blend});

        m_spectrumPipeline->create();
    }

    // Spectrum fill pipeline (fragment shader per-pixel spectrum)
    {
        m_spectrumFillSrb.reset(m_rhi->newShaderResourceBindings());
        m_spectrumFillSrb->setBindings(
            {QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::FragmentStage,
                                                      m_spectrumFillUniformBuffer.get()),
             QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                       m_spectrumDataTexture.get(), m_sampler.get())});
        m_spectrumFillSrb->create();

        m_spectrumFillPipeline.reset(m_rhi->newGraphicsPipeline());
        m_spectrumFillPipeline->setShaderStages(
            {{QRhiShaderStage::Vertex, m_spectrumFillVert}, {QRhiShaderStage::Fragment, m_spectrumFillFrag}});

        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({{4 * sizeof(float)}});                         // position(2) + texcoord(2)
        inputLayout.setAttributes({{0, 0, QRhiVertexInputAttribute::Float2, 0}, // position
                                   {0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float)}}); // texcoord
        m_spectrumFillPipeline->setVertexInputLayout(inputLayout);
        m_spectrumFillPipeline->setTopology(QRhiGraphicsPipeline::Triangles);
        m_spectrumFillPipeline->setShaderResourceBindings(m_spectrumFillSrb.get());
        m_spectrumFillPipeline->setRenderPassDescriptor(m_rpDesc);

        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
        blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        m_spectrumFillPipeline->setTargetBlends({blend});

        m_spectrumFillPipeline->create();
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
            m_waterfallData.constData() + m_waterfallWriteRow * TEXTURE_WIDTH, TEXTURE_WIDTH);
        rowUpload.setDestinationTopLeft(QPoint(0, m_waterfallWriteRow));
        rowUpload.setSourceSize(QSize(TEXTURE_WIDTH, 1));
        rub->uploadTexture(m_waterfallTexture.get(), QRhiTextureUploadEntry(0, 0, rowUpload));
        m_waterfallWriteRow = (m_waterfallWriteRow + 1) % WATERFALL_HISTORY;
        m_waterfallNeedsUpdate = false;
    }

    // Update spectrum uniform buffer
    struct {
        float viewportWidth;
        float viewportHeight;
        float padding[2];
    } spectrumUniforms = {w, spectrumHeight, {0, 0}};
    rub->updateDynamicBuffer(m_spectrumUniformBuffer.get(), 0, sizeof(spectrumUniforms), &spectrumUniforms);

    // Update waterfall uniform buffer
    float scrollOffset = static_cast<float>(m_waterfallWriteRow) / WATERFALL_HISTORY;
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
        QVector<float> normalizedSpectrum(TEXTURE_WIDTH);
        for (int i = 0; i < TEXTURE_WIDTH; ++i) {
            float srcPos = static_cast<float>(i) / (TEXTURE_WIDTH - 1) * (m_currentSpectrum.size() - 1);
            int idx = qBound(0, qRound(srcPos), m_currentSpectrum.size() - 1);
            float normalized = normalizeDb(m_currentSpectrum[idx]);
            float adjusted = qMax(0.0f, normalized - m_smoothedBaseline);
            normalizedSpectrum[i] = adjusted * 0.95f;
        }

        QRhiTextureSubresourceUploadDescription specDataUpload(normalizedSpectrum.constData(),
                                                               normalizedSpectrum.size() * sizeof(float));
        specDataUpload.setSourceSize(QSize(TEXTURE_WIDTH, 1));
        rub->uploadTexture(m_spectrumDataTexture.get(), QRhiTextureUploadEntry(0, 0, specDataUpload));

        // Update spectrum fill uniform buffer (before beginPass)
        // Colors: gradient from visible green at noise floor to bright lime at peaks
        struct {
            float spectrumHeight;
            float frostHeight;
            float padding1;
            float padding2;
            float peakColor[4];
            float baseColor[4];
        } specFillUniforms = {
            spectrumHeight,
            40.0f,
            0.0f,
            0.0f,
            {100.0f / 255.0f, 255.0f / 255.0f, 100.0f / 255.0f, 1.0f}, // Bright lime at peaks
            {30.0f / 255.0f, 100.0f / 255.0f, 30.0f / 255.0f, 0.7f}    // Visible green at base
        };
        rub->updateDynamicBuffer(m_spectrumFillUniformBuffer.get(), 0, sizeof(specFillUniforms), &specFillUniforms);
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

    // Peak trail disabled - now using line-only spectrum rendering
    // TODO: Re-implement peak trail as a separate dimmer line if needed
    if (false && m_spectrumPipeline && m_peakHoldEnabled && !m_peakHold.isEmpty() &&
        m_peakHold.size() == m_currentSpectrum.size()) {
        QVector<float> peakTrailVerts;
        peakTrailVerts.reserve(m_peakHold.size() * 6);

        // Dim lime green for peak trail (matches spectrum theme)
        const float trailR = 60.0f / 255.0f;
        const float trailG = 140.0f / 255.0f;
        const float trailB = 60.0f / 255.0f;

        for (int i = 0; i < m_peakHold.size(); ++i) {
            float x = (static_cast<float>(i) / (m_peakHold.size() - 1)) * w;
            float peakNorm = normalizeDb(m_peakHold[i]);
            float currNorm = normalizeDb(m_currentSpectrum[i]);

            // Only draw trail where peak > current (the "ghost" part)
            float peakAdj = qMax(0.0f, peakNorm - m_smoothedBaseline);
            float currAdj = qMax(0.0f, currNorm - m_smoothedBaseline);

            if (peakAdj > currAdj + 0.02f) { // Threshold to avoid flicker
                float peakY = spectrumHeight * (1.0f - peakAdj * 0.95f);

                // Calculate alpha based on difference (larger diff = more visible)
                float diff = peakAdj - currAdj;
                float alpha = qBound(0.3f, diff * 3.0f, 0.9f);

                // Single vertex at peak hold position
                peakTrailVerts.append(x);
                peakTrailVerts.append(peakY);
                peakTrailVerts.append(trailR);
                peakTrailVerts.append(trailG);
                peakTrailVerts.append(trailB);
                peakTrailVerts.append(alpha * 0.6f);
            }
        }

        if (!peakTrailVerts.isEmpty()) {
            QRhiResourceUpdateBatch *peakRub = m_rhi->nextResourceUpdateBatch();
            peakRub->updateDynamicBuffer(m_spectrumVbo.get(), 0, peakTrailVerts.size() * sizeof(float),
                                         peakTrailVerts.constData());
            cb->resourceUpdate(peakRub);

            cb->setViewport({0, waterfallHeight, w, spectrumHeight});
            cb->setGraphicsPipeline(m_spectrumPipeline.get());
            cb->setShaderResources(m_spectrumSrb.get());
            const QRhiCommandBuffer::VertexInput peakVbufBinding(m_spectrumVbo.get(), 0);
            cb->setVertexInput(0, 1, &peakVbufBinding);
            cb->draw(peakTrailVerts.size() / 6); // 6 floats per vertex (pos2 + color4)
        }
    }

    // Draw spectrum fill using fragment shader (per-pixel spectrum sampling)
    // (Texture and uniform buffer were uploaded before beginPass)
    if (m_spectrumFillPipeline && !m_currentSpectrum.isEmpty()) {
        cb->setViewport({0, waterfallHeight, w, spectrumHeight});
        cb->setGraphicsPipeline(m_spectrumFillPipeline.get());
        cb->setShaderResources(m_spectrumFillSrb.get());
        const QRhiCommandBuffer::VertexInput specFillVbufBinding(m_spectrumFillVbo.get(), 0);
        cb->setVertexInput(0, 1, &specFillVbufBinding);
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

        // Draw grid (spectrum area only - top portion, Y=0 to spectrumHeight)
        if (m_gridEnabled) {
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

            drawLines(gridVerts, m_gridColor);
        }

        // Draw passband overlay (uses separate buffers to avoid GPU conflicts)
        if (m_cursorVisible && m_filterBw > 0 && m_tunedFreq > 0) {
            // Calculate passband edges based on mode
            qint64 lowFreq, highFreq;

            if (m_mode == "LSB") {
                highFreq = m_tunedFreq;
                lowFreq = m_tunedFreq - m_filterBw;
            } else if (m_mode == "USB" || m_mode == "DATA" || m_mode == "DATA-R") {
                lowFreq = m_tunedFreq;
                highFreq = m_tunedFreq + m_filterBw;
            } else if (m_mode == "CW" || m_mode == "CW-R") {
                // CW centers on pitch offset
                int pitchOffset = (m_mode == "CW") ? m_cwPitch : -m_cwPitch;
                qint64 center = m_tunedFreq + pitchOffset;
                lowFreq = center - m_filterBw / 2;
                highFreq = center + m_filterBw / 2;
            } else {
                // AM/FM - symmetric around tuned freq
                lowFreq = m_tunedFreq - m_filterBw / 2;
                highFreq = m_tunedFreq + m_filterBw / 2;
            }

            // Convert to pixel coordinates
            float x1 = freqToNormalized(lowFreq) * w;
            float x2 = freqToNormalized(highFreq) * w;

            // Clamp to visible area
            x1 = qBound(0.0f, x1, w);
            x2 = qBound(0.0f, x2, w);

            if (x2 > x1) {
                // Draw passband quad using SEPARATE buffers (m_passbandVbo, m_passbandUniformBuffer)
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

            // Draw frequency marker (vertical line at tuned freq) - uses SEPARATE marker buffers
            // Use spectrumHeight not h - marker should only appear in spectrum area, not waterfall
            // Account for CW pitch offset so marker appears at center of passband
            qint64 markerFreq = m_tunedFreq;
            if (m_mode == "CW") {
                markerFreq = m_tunedFreq + m_cwPitch;
            } else if (m_mode == "CW-R") {
                markerFreq = m_tunedFreq - m_cwPitch;
            }
            float markerX = freqToNormalized(markerFreq) * w;
            if (markerX >= 0 && markerX <= w) {
                QVector<float> markerVerts = {markerX, 0.0f, markerX, spectrumHeight};

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
                cb->setGraphicsPipeline(m_overlayLinePipeline.get());
                cb->setShaderResources(m_markerSrb.get());
                const QRhiCommandBuffer::VertexInput mkVbufBinding(m_markerVbo.get(), 0);
                cb->setVertexInput(0, 1, &mkVbufBinding);
                cb->draw(2);
            }

            // Draw notch filter marker (red line) - reuses overlay buffers since drawn last
            if (m_notchEnabled && m_notchPitchHz > 0) {
                qint64 notchFreq;
                if (m_mode == "LSB") {
                    notchFreq = m_tunedFreq - m_notchPitchHz;
                } else {
                    notchFreq = m_tunedFreq + m_notchPitchHz;
                }

                float notchX = freqToNormalized(notchFreq) * w;
                if (notchX >= 0 && notchX <= w) {
                    QVector<float> notchVerts = {notchX, 0.0f, notchX, spectrumHeight};

                    QRhiResourceUpdateBatch *notchRub = m_rhi->nextResourceUpdateBatch();
                    notchRub->updateDynamicBuffer(m_overlayVbo.get(), 0, notchVerts.size() * sizeof(float),
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
                    notchRub->updateDynamicBuffer(m_overlayUniformBuffer.get(), 0, sizeof(notchUniforms),
                                                  &notchUniforms);

                    cb->resourceUpdate(notchRub);
                    cb->setGraphicsPipeline(m_overlayLinePipeline.get());
                    cb->setShaderResources(m_overlaySrb.get());
                    const QRhiCommandBuffer::VertexInput notchVbufBinding(m_overlayVbo.get(), 0);
                    cb->setVertexInput(0, 1, &notchVbufBinding);
                    cb->draw(2);
                }
            }
        }
    }

    cb->endPass();

    // Use QPainter for text overlays
    // Note: QPainter on QRhiWidget works after endPass
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
    m_currentSpectrum = m_rawSpectrum;

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
    m_currentSpectrum = m_rawSpectrum;
    m_waterfallNeedsUpdate = true;
    update();
}

void PanadapterRhiWidget::decompressBins(const QByteArray &bins, QVector<float> &out) {
    out.resize(bins.size());
    for (int i = 0; i < bins.size(); ++i) {
        out[i] = static_cast<quint8>(bins[i]) - 160.0f;
    }
}

void PanadapterRhiWidget::updateWaterfallData() {
    if (m_currentSpectrum.isEmpty())
        return;

    // Resample spectrum to texture width
    int row = m_waterfallWriteRow;
    for (int i = 0; i < TEXTURE_WIDTH; ++i) {
        float srcPos = static_cast<float>(i) / (TEXTURE_WIDTH - 1) * (m_currentSpectrum.size() - 1);
        int idx = qBound(0, qRound(srcPos), m_currentSpectrum.size() - 1);
        float normalized = normalizeDb(m_currentSpectrum[idx]);
        m_waterfallData[row * TEXTURE_WIDTH + i] =
            static_cast<quint8>(qBound(0, static_cast<int>(normalized * 255), 255));
    }
}

float PanadapterRhiWidget::normalizeDb(float db) {
    return qBound(0.0f, (db - m_minDb) / (m_maxDb - m_minDb), 1.0f);
}

float PanadapterRhiWidget::freqToNormalized(qint64 freq) {
    qint64 startFreq = m_centerFreq - m_spanHz / 2;
    return static_cast<float>(freq - startFreq) / m_spanHz;
}

qint64 PanadapterRhiWidget::xToFreq(int x, int w) {
    qint64 startFreq = m_centerFreq - m_spanHz / 2;
    return startFreq + (static_cast<qint64>(x) * m_spanHz) / w;
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
        m_minDb = static_cast<float>(level - 28);
        m_maxDb = static_cast<float>(level + 52);
        update();
    }
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
