#include "panadapter.h"
#include <QMouseEvent>
#include <QtMath>
#include <cmath>

// Shader source code (GLSL 120 for macOS compatibility)
static const char *waterfallVertexShader = R"(
attribute vec2 position;
attribute vec2 texCoord;
varying vec2 fragTexCoord;
uniform float scrollOffset;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    fragTexCoord = vec2(texCoord.x, texCoord.y + scrollOffset);
}
)";

static const char *waterfallFragmentShader = R"(
varying vec2 fragTexCoord;
uniform sampler2D waterfallTex;
uniform sampler2D colorLutTex;

void main() {
    float dbValue = texture2D(waterfallTex, fragTexCoord).r;
    gl_FragColor = texture2D(colorLutTex, vec2(dbValue, 0.5));
}
)";

static const char *spectrumVertexShader = R"(
attribute vec2 position;
uniform vec2 viewportSize;

void main() {
    vec2 ndc = (position / viewportSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;  // Flip Y for Qt coordinate system
    gl_Position = vec4(ndc, 0.0, 1.0);
}
)";

static const char *spectrumFragmentShader = R"(
uniform vec4 lineColor;

void main() {
    gl_FragColor = lineColor;
}
)";

static const char *overlayVertexShader = R"(
attribute vec2 position;
uniform vec2 viewportSize;

void main() {
    vec2 ndc = (position / viewportSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    gl_Position = vec4(ndc, 0.0, 1.0);
}
)";

static const char *overlayFragmentShader = R"(
uniform vec4 color;

void main() {
    gl_FragColor = color;
}
)";

PanadapterWidget::PanadapterWidget(QWidget *parent)
    : QOpenGLWidget(parent), m_spectrumVBO(QOpenGLBuffer::VertexBuffer), m_waterfallVBO(QOpenGLBuffer::VertexBuffer),
      m_overlayVBO(QOpenGLBuffer::VertexBuffer) {
    setMinimumHeight(200);
    setMouseTracking(true);

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

PanadapterWidget::~PanadapterWidget() {
    makeCurrent();
    if (m_waterfallTexture)
        glDeleteTextures(1, &m_waterfallTexture);
    if (m_colorLutTexture)
        glDeleteTextures(1, &m_colorLutTexture);
    delete m_waterfallShader;
    delete m_spectrumShader;
    delete m_overlayShader;
    doneCurrent();
}

void PanadapterWidget::initializeGL() {
    initializeOpenGLFunctions();

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initColorLUT();
    initShaders();
    initTextures();
    initBuffers();

    m_glInitialized = true;
}

void PanadapterWidget::initShaders() {
    // Waterfall shader
    m_waterfallShader = new QOpenGLShaderProgram(this);
    m_waterfallShader->addShaderFromSourceCode(QOpenGLShader::Vertex, waterfallVertexShader);
    m_waterfallShader->addShaderFromSourceCode(QOpenGLShader::Fragment, waterfallFragmentShader);
    m_waterfallShader->link();

    // Spectrum shader
    m_spectrumShader = new QOpenGLShaderProgram(this);
    m_spectrumShader->addShaderFromSourceCode(QOpenGLShader::Vertex, spectrumVertexShader);
    m_spectrumShader->addShaderFromSourceCode(QOpenGLShader::Fragment, spectrumFragmentShader);
    m_spectrumShader->link();

    // Overlay shader (grid, markers)
    m_overlayShader = new QOpenGLShaderProgram(this);
    m_overlayShader->addShaderFromSourceCode(QOpenGLShader::Vertex, overlayVertexShader);
    m_overlayShader->addShaderFromSourceCode(QOpenGLShader::Fragment, overlayFragmentShader);
    m_overlayShader->link();
}

void PanadapterWidget::initTextures() {
    // Waterfall texture (256 rows x textureWidth, single channel for dB values)
    glGenTextures(1, &m_waterfallTexture);
    glBindTexture(GL_TEXTURE_2D, m_waterfallTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // Wrap for scrolling

    // Allocate texture storage (LUMINANCE for OpenGL 2.1 compatibility)
    QVector<GLubyte> zeros(m_textureWidth * WATERFALL_HISTORY, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_textureWidth, WATERFALL_HISTORY, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                 zeros.data());

    // Color LUT texture (256x1 2D texture for GLSL 120 compatibility)
    glGenTextures(1, &m_colorLutTexture);
    glBindTexture(GL_TEXTURE_2D, m_colorLutTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_colorLUT.data());
}

void PanadapterWidget::initBuffers() {
    m_vao.create();
    m_vao.bind();

    // Waterfall quad vertices (full screen quad in NDC)
    // Texture coord t ranges from 0 to (WATERFALL_HISTORY-1)/WATERFALL_HISTORY
    // This prevents top/bottom from sampling the same row due to GL_REPEAT wrapping
    // With scrollOffset addition: top shows newest row, bottom shows oldest
    float tMax = static_cast<float>(WATERFALL_HISTORY - 1) / WATERFALL_HISTORY;
    float waterfallQuad[] = {
        // position (x, y), texcoord (s, t)
        -1.0f, -1.0f, 0.0f, 0.0f, // bottom-left, t=0 (oldest)
        1.0f,  -1.0f, 1.0f, 0.0f, // bottom-right
        1.0f,  1.0f,  1.0f, tMax, // top-right, t=tMax (newest)
        -1.0f, -1.0f, 0.0f, 0.0f, // bottom-left
        1.0f,  1.0f,  1.0f, tMax, // top-right
        -1.0f, 1.0f,  0.0f, tMax  // top-left, t=tMax (newest)
    };

    m_waterfallVBO.create();
    m_waterfallVBO.bind();
    m_waterfallVBO.allocate(waterfallQuad, sizeof(waterfallQuad));

    // Spectrum VBO - will be resized dynamically
    m_spectrumVBO.create();
    m_spectrumVBO.setUsagePattern(QOpenGLBuffer::DynamicDraw);

    // Overlay VBO - for grid lines and markers
    m_overlayVBO.create();
    m_overlayVBO.setUsagePattern(QOpenGLBuffer::DynamicDraw);

    m_vao.release();
}

void PanadapterWidget::initColorLUT() {
    // Create 256-entry RGBA color LUT for waterfall
    // 8-stage color progression: Black → Dark Blue → Royal Blue → Cyan → Green → Yellow → Red
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

        m_colorLUT[i * 4 + 0] = static_cast<GLubyte>(qBound(0, r, 255));
        m_colorLUT[i * 4 + 1] = static_cast<GLubyte>(qBound(0, g, 255));
        m_colorLUT[i * 4 + 2] = static_cast<GLubyte>(qBound(0, b, 255));
        m_colorLUT[i * 4 + 3] = 255;
    }
}

void PanadapterWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void PanadapterWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);

    // On HiDPI/Retina displays, framebuffer size differs from logical widget size
    // Must use device pixel ratio for correct OpenGL viewport sizing
    qreal dpr = devicePixelRatioF();
    int w = static_cast<int>(width() * dpr);
    int h = static_cast<int>(height() * dpr);
    int spectrumHeight = static_cast<int>(h * m_spectrumRatio);
    int waterfallHeight = h - spectrumHeight;

    // In OpenGL, Y=0 is at BOTTOM of framebuffer
    // - Spectrum at top: viewport starts at waterfallHeight
    // - Waterfall below: viewport starts at 0

    // Draw spectrum (top portion of widget)
    glViewport(0, waterfallHeight, w, spectrumHeight);
    drawSpectrum(spectrumHeight);

    // Draw waterfall (bottom portion of widget)
    glViewport(0, 0, w, waterfallHeight);
    drawWaterfall();

    // Draw overlays (grid, markers) - full viewport
    glViewport(0, 0, w, h);
    drawOverlays();
}

void PanadapterWidget::drawWaterfall() {
    if (!m_waterfallShader || m_currentSpectrum.isEmpty())
        return;

    m_waterfallShader->bind();

    // Bind waterfall texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_waterfallTexture);
    m_waterfallShader->setUniformValue("waterfallTex", 0);

    // Bind color LUT texture (2D texture for GLSL 120 compatibility)
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_colorLutTexture);
    m_waterfallShader->setUniformValue("colorLutTex", 1);

    // Set scroll offset for circular buffer
    float scrollOffset = static_cast<float>(m_waterfallWriteRow) / WATERFALL_HISTORY;
    m_waterfallShader->setUniformValue("scrollOffset", scrollOffset);

    // Draw waterfall quad
    m_waterfallVBO.bind();
    int posLoc = m_waterfallShader->attributeLocation("position");
    int texLoc = m_waterfallShader->attributeLocation("texCoord");
    m_waterfallShader->enableAttributeArray(posLoc);
    m_waterfallShader->enableAttributeArray(texLoc);
    m_waterfallShader->setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 4 * sizeof(float));
    m_waterfallShader->setAttributeBuffer(texLoc, GL_FLOAT, 2 * sizeof(float), 2, 4 * sizeof(float));

    glDrawArrays(GL_TRIANGLES, 0, 6);

    m_waterfallShader->disableAttributeArray(posLoc);
    m_waterfallShader->disableAttributeArray(texLoc);
    m_waterfallShader->release();
}

void PanadapterWidget::drawSpectrum(int spectrumHeight) {
    if (!m_spectrumShader || m_currentSpectrum.isEmpty())
        return;

    // Use DPR-scaled width to match viewport
    qreal dpr = devicePixelRatioF();
    int w = static_cast<int>(width() * dpr);

    // Build spectrum line vertices
    QVector<float> vertices;
    vertices.reserve(m_currentSpectrum.size() * 2);

    // Find smoothed baseline for stable display
    float frameMinNormalized = 1.0f;
    for (int i = 0; i < m_currentSpectrum.size(); ++i) {
        float normalized = normalizeDb(m_currentSpectrum[i]);
        if (normalized < frameMinNormalized) {
            frameMinNormalized = normalized;
        }
    }

    const float baselineAlpha = 0.05f;
    if (m_smoothedBaseline < 0.001f) {
        m_smoothedBaseline = frameMinNormalized;
    } else {
        m_smoothedBaseline = baselineAlpha * frameMinNormalized + (1.0f - baselineAlpha) * m_smoothedBaseline;
    }

    for (int i = 0; i < m_currentSpectrum.size(); ++i) {
        float x = (static_cast<float>(i) / (m_currentSpectrum.size() - 1)) * w;
        float normalized = normalizeDb(m_currentSpectrum[i]);
        float adjusted = qMax(0.0f, normalized - m_smoothedBaseline);
        float y = spectrumHeight * (1.0f - adjusted * 0.95f);

        vertices.append(x);
        vertices.append(y);
    }

    // Upload vertices
    m_spectrumVBO.bind();
    m_spectrumVBO.allocate(vertices.data(), vertices.size() * sizeof(float));

    // Draw spectrum line
    m_spectrumShader->bind();
    m_spectrumShader->setUniformValue("viewportSize", QVector2D(w, spectrumHeight));
    m_spectrumShader->setUniformValue("lineColor", QVector4D(0.2f, 1.0f, 0.2f, 1.0f)); // Lime green

    int posLoc = m_spectrumShader->attributeLocation("position");
    m_spectrumShader->enableAttributeArray(posLoc);
    m_spectrumShader->setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));

    glLineWidth(1.5f);
    glDrawArrays(GL_LINE_STRIP, 0, vertices.size() / 2);

    // Draw peak hold if enabled
    if (m_peakHoldEnabled && !m_peakHold.isEmpty()) {
        QVector<float> peakVertices;
        peakVertices.reserve(m_peakHold.size() * 2);

        for (int i = 0; i < m_peakHold.size(); ++i) {
            float x = (static_cast<float>(i) / (m_peakHold.size() - 1)) * w;
            float normalized = normalizeDb(m_peakHold[i]);
            float adjusted = qMax(0.0f, normalized - m_smoothedBaseline);
            float y = spectrumHeight * (1.0f - adjusted * 0.95f);

            peakVertices.append(x);
            peakVertices.append(y);
        }

        m_spectrumVBO.allocate(peakVertices.data(), peakVertices.size() * sizeof(float));
        m_spectrumShader->setUniformValue("lineColor", QVector4D(1.0f, 1.0f, 1.0f, 0.4f)); // White, semi-transparent
        glDrawArrays(GL_LINE_STRIP, 0, peakVertices.size() / 2);
    }

    m_spectrumShader->disableAttributeArray(posLoc);
    m_spectrumShader->release();
}

void PanadapterWidget::drawOverlays() {
    if (!m_overlayShader)
        return;

    // Use DPR-scaled dimensions to match viewport
    qreal dpr = devicePixelRatioF();
    int w = static_cast<int>(width() * dpr);
    int h = static_cast<int>(height() * dpr);
    int spectrumHeight = static_cast<int>(h * m_spectrumRatio);

    m_overlayShader->bind();
    m_overlayShader->setUniformValue("viewportSize", QVector2D(w, h));

    int posLoc = m_overlayShader->attributeLocation("position");
    QVector<float> lines;

    // Grid lines (spectrum area only)
    if (m_gridEnabled) {
        m_overlayShader->setUniformValue("color", QVector4D(1.0f, 1.0f, 1.0f, 0.15f));

        // Vertical grid lines
        int vertDivisions = qMax(12, w / 50);
        for (int i = 0; i <= vertDivisions; ++i) {
            float x = (w * i) / vertDivisions;
            lines.append(x);
            lines.append(0);
            lines.append(x);
            lines.append(static_cast<float>(spectrumHeight));
        }

        // Horizontal grid lines
        int horzDivisions = qMax(8, spectrumHeight / 40);
        for (int i = 0; i <= horzDivisions; ++i) {
            float y = (spectrumHeight * i) / horzDivisions;
            lines.append(0);
            lines.append(y);
            lines.append(static_cast<float>(w));
            lines.append(y);
        }

        if (!lines.isEmpty()) {
            m_overlayVBO.bind();
            m_overlayVBO.allocate(lines.data(), lines.size() * sizeof(float));
            m_overlayShader->enableAttributeArray(posLoc);
            m_overlayShader->setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));
            glLineWidth(1.0f);
            glDrawArrays(GL_LINES, 0, lines.size() / 2);
        }
    }

    // Filter passband
    if (m_cursorVisible && m_filterBw > 0 && m_centerFreq > 0) {
        float carrierNorm = freqToNormalized(m_tunedFreq);
        float bwNorm = static_cast<float>(m_filterBw) / m_spanHz;

        float passbandLeft, passbandRight;
        if (m_mode == "CW" || m_mode == "CW-R") {
            int shiftHz = m_ifShift * 10;
            int shiftOffsetHz = shiftHz - m_cwPitch;
            float shiftNorm = static_cast<float>(shiftOffsetHz) / m_spanHz;
            float centerNorm = carrierNorm + shiftNorm;
            passbandLeft = centerNorm - bwNorm / 2;
            passbandRight = centerNorm + bwNorm / 2;
        } else if (m_mode == "LSB") {
            passbandLeft = carrierNorm - bwNorm;
            passbandRight = carrierNorm;
        } else {
            passbandLeft = carrierNorm;
            passbandRight = carrierNorm + bwNorm;
        }

        // Draw passband quad (semi-transparent blue)
        float x1 = passbandLeft * w;
        float x2 = passbandRight * w;

        m_overlayShader->setUniformValue(
            "color", QVector4D(m_passbandColor.redF(), m_passbandColor.greenF(), m_passbandColor.blueF(), 0.25f));

        QVector<float> quad = {
            x1, 0, x2, 0, x2, static_cast<float>(h), x1, 0, x2, static_cast<float>(h), x1, static_cast<float>(h)};

        m_overlayVBO.bind();
        m_overlayVBO.allocate(quad.data(), quad.size() * sizeof(float));
        m_overlayShader->enableAttributeArray(posLoc);
        m_overlayShader->setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // Frequency marker (tuned frequency line)
    if (m_centerFreq > 0) {
        float markerNorm = freqToNormalized(m_tunedFreq);
        float x = markerNorm * w;

        m_overlayShader->setUniformValue("color",
                                         QVector4D(m_frequencyMarkerColor.redF(), m_frequencyMarkerColor.greenF(),
                                                   m_frequencyMarkerColor.blueF(), 1.0f));

        QVector<float> markerLine = {x, 0, x, static_cast<float>(h)};
        m_overlayVBO.bind();
        m_overlayVBO.allocate(markerLine.data(), markerLine.size() * sizeof(float));
        m_overlayShader->enableAttributeArray(posLoc);
        m_overlayShader->setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));
        glLineWidth(2.0f);
        glDrawArrays(GL_LINES, 0, 2);
    }

    // Notch filter marker
    if (m_notchEnabled && m_notchPitchHz > 0) {
        qint64 notchFreq;
        if (m_mode == "LSB") {
            notchFreq = m_tunedFreq - m_notchPitchHz;
        } else {
            notchFreq = m_tunedFreq + m_notchPitchHz;
        }

        float notchNorm = freqToNormalized(notchFreq);
        float x = notchNorm * w;

        m_overlayShader->setUniformValue("color", QVector4D(1.0f, 0.0f, 0.0f, 1.0f)); // Red

        QVector<float> notchLine = {x, 0, x, static_cast<float>(h)};
        m_overlayVBO.bind();
        m_overlayVBO.allocate(notchLine.data(), notchLine.size() * sizeof(float));
        m_overlayShader->enableAttributeArray(posLoc);
        m_overlayShader->setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));
        glLineWidth(2.0f);
        glDrawArrays(GL_LINES, 0, 2);
    }

    m_overlayShader->disableAttributeArray(posLoc);
    m_overlayShader->release();
}

void PanadapterWidget::updateSpectrum(const QByteArray &bins, qint64 centerFreq, qint32 sampleRate, float noiseFloor) {
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

    // Update waterfall texture
    updateWaterfallTexture(m_currentSpectrum);

    update();
}

void PanadapterWidget::updateMiniSpectrum(const QByteArray &bins) {
    m_rawSpectrum.resize(bins.size());
    for (int i = 0; i < bins.size(); ++i) {
        m_rawSpectrum[i] = static_cast<quint8>(bins[i]) * 10.0f - 160.0f;
    }

    applySmoothing(m_rawSpectrum, m_currentSpectrum);
    updateWaterfallTexture(m_currentSpectrum);
    update();
}

void PanadapterWidget::decompressBins(const QByteArray &bins, QVector<float> &out) {
    out.resize(bins.size());
    for (int i = 0; i < bins.size(); ++i) {
        out[i] = static_cast<quint8>(bins[i]) - 160.0f;
    }
}

void PanadapterWidget::applySmoothing(const QVector<float> &input, QVector<float> &output) {
    if (output.size() != input.size()) {
        output = input;
        return;
    }

    for (int i = 0; i < input.size(); ++i) {
        output[i] = m_smoothingAlpha * input[i] + (1.0f - m_smoothingAlpha) * output[i];
    }
}

void PanadapterWidget::updateWaterfallTexture(const QVector<float> &spectrum) {
    if (spectrum.isEmpty() || !m_glInitialized)
        return;

    makeCurrent();

    // Resample spectrum to texture width
    QVector<GLubyte> row(m_textureWidth);
    for (int i = 0; i < m_textureWidth; ++i) {
        float srcPos = static_cast<float>(i) / (m_textureWidth - 1) * (spectrum.size() - 1);
        int idx = qBound(0, qRound(srcPos), spectrum.size() - 1);
        float normalized = normalizeDb(spectrum[idx]);
        row[i] = static_cast<GLubyte>(qBound(0, static_cast<int>(normalized * 255), 255));
    }

    // Upload single row to texture
    glBindTexture(GL_TEXTURE_2D, m_waterfallTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, m_waterfallWriteRow, m_textureWidth, 1, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                    row.data());

    m_waterfallWriteRow = (m_waterfallWriteRow + 1) % WATERFALL_HISTORY;

    doneCurrent();
}

float PanadapterWidget::normalizeDb(float db) {
    return qBound(0.0f, (db - m_minDb) / (m_maxDb - m_minDb), 1.0f);
}

float PanadapterWidget::freqToNormalized(qint64 freq) {
    qint64 startFreq = m_centerFreq - m_spanHz / 2;
    return static_cast<float>(freq - startFreq) / m_spanHz;
}

qint64 PanadapterWidget::xToFreq(int x, int w) {
    qint64 startFreq = m_centerFreq - m_spanHz / 2;
    return startFreq + (static_cast<qint64>(x) * m_spanHz) / w;
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
        m_showWaterfallMarker = true;
        m_waterfallMarkerTimer->start(500);
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

void PanadapterWidget::setPassbandColor(const QColor &color) {
    m_passbandColor = color;
    update();
}

void PanadapterWidget::setFrequencyMarkerColor(const QColor &color) {
    m_frequencyMarkerColor = color;
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

void PanadapterWidget::setCursorVisible(bool visible) {
    if (m_cursorVisible != visible) {
        m_cursorVisible = visible;
        update();
    }
}

void PanadapterWidget::clear() {
    m_currentSpectrum.clear();
    m_rawSpectrum.clear();
    m_peakHold.clear();
    m_waterfallWriteRow = 0;

    if (m_glInitialized) {
        makeCurrent();
        QVector<GLubyte> zeros(m_textureWidth * WATERFALL_HISTORY, 0);
        glBindTexture(GL_TEXTURE_2D, m_waterfallTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_textureWidth, WATERFALL_HISTORY, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                        zeros.data());
        doneCurrent();
    }

    update();
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
    int degrees = event->angleDelta().y() / 8;
    int steps = degrees / 15;

    if (steps != 0) {
        emit frequencyScrolled(steps);
    }
    event->accept();
}
