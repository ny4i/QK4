#include "minipanwidget.h"
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

MiniPanWidget::MiniPanWidget(QWidget *parent)
    : QOpenGLWidget(parent), m_spectrumVBO(QOpenGLBuffer::VertexBuffer), m_waterfallVBO(QOpenGLBuffer::VertexBuffer),
      m_overlayVBO(QOpenGLBuffer::VertexBuffer) {
    setFixedHeight(110);
    setMinimumWidth(180);
    setMaximumWidth(200);
}

MiniPanWidget::~MiniPanWidget() {
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

void MiniPanWidget::initializeGL() {
    initializeOpenGLFunctions();

    glClearColor(0.04f, 0.04f, 0.04f, 1.0f); // Dark background
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initColorLUT();
    initShaders();
    initTextures();
    initBuffers();

    m_glInitialized = true;
}

void MiniPanWidget::initShaders() {
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

    // Overlay shader
    m_overlayShader = new QOpenGLShaderProgram(this);
    m_overlayShader->addShaderFromSourceCode(QOpenGLShader::Vertex, overlayVertexShader);
    m_overlayShader->addShaderFromSourceCode(QOpenGLShader::Fragment, overlayFragmentShader);
    m_overlayShader->link();
}

void MiniPanWidget::initTextures() {
    // Waterfall texture (WATERFALL_HISTORY rows x TEXTURE_WIDTH, single channel)
    glGenTextures(1, &m_waterfallTexture);
    glBindTexture(GL_TEXTURE_2D, m_waterfallTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    QVector<GLubyte> zeros(TEXTURE_WIDTH * WATERFALL_HISTORY, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, TEXTURE_WIDTH, WATERFALL_HISTORY, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                 zeros.data());

    // Color LUT texture (256x1 2D texture)
    glGenTextures(1, &m_colorLutTexture);
    glBindTexture(GL_TEXTURE_2D, m_colorLutTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_colorLUT.data());
}

void MiniPanWidget::initBuffers() {
    m_vao.create();
    m_vao.bind();

    // Waterfall quad vertices
    // Texture coord t ranges from 0 to (WATERFALL_HISTORY-1)/WATERFALL_HISTORY
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

    m_waterfallVBO.create();
    m_waterfallVBO.bind();
    m_waterfallVBO.allocate(waterfallQuad, sizeof(waterfallQuad));

    m_spectrumVBO.create();
    m_spectrumVBO.setUsagePattern(QOpenGLBuffer::DynamicDraw);

    m_overlayVBO.create();
    m_overlayVBO.setUsagePattern(QOpenGLBuffer::DynamicDraw);

    m_vao.release();
}

void MiniPanWidget::initColorLUT() {
    // Create 256-entry RGBA color lookup table
    m_colorLUT.resize(256 * 4);

    for (int i = 0; i < 256; ++i) {
        float t = i / 255.0f;
        int r, g, b;

        if (t < 0.2f) {
            // Black to dark blue
            float s = t / 0.2f;
            r = 0;
            g = 0;
            b = static_cast<int>(60 * s);
        } else if (t < 0.4f) {
            // Dark blue to cyan
            float s = (t - 0.2f) / 0.2f;
            r = 0;
            g = static_cast<int>(180 * s);
            b = 60 + static_cast<int>(140 * s);
        } else if (t < 0.6f) {
            // Cyan to yellow
            float s = (t - 0.4f) / 0.2f;
            r = static_cast<int>(255 * s);
            g = 180 + static_cast<int>(75 * s);
            b = 200 - static_cast<int>(200 * s);
        } else if (t < 0.8f) {
            // Yellow to orange/red
            float s = (t - 0.6f) / 0.2f;
            r = 255;
            g = 255 - static_cast<int>(155 * s);
            b = 0;
        } else {
            // Orange/red to white
            float s = (t - 0.8f) / 0.2f;
            r = 255;
            g = 100 + static_cast<int>(155 * s);
            b = static_cast<int>(255 * s);
        }

        m_colorLUT[i * 4 + 0] = static_cast<GLubyte>(qBound(0, r, 255));
        m_colorLUT[i * 4 + 1] = static_cast<GLubyte>(qBound(0, g, 255));
        m_colorLUT[i * 4 + 2] = static_cast<GLubyte>(qBound(0, b, 255));
        m_colorLUT[i * 4 + 3] = 255;
    }
}

void MiniPanWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void MiniPanWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Account for device pixel ratio on HiDPI displays
    qreal dpr = devicePixelRatioF();
    int w = static_cast<int>(width() * dpr);
    int h = static_cast<int>(height() * dpr);
    int spectrumHeight = static_cast<int>(h * m_spectrumRatio);
    int waterfallHeight = h - spectrumHeight;

    // Draw spectrum (top portion)
    glViewport(0, waterfallHeight, w, spectrumHeight);
    drawSpectrumGL(spectrumHeight);

    // Draw waterfall (bottom portion)
    glViewport(0, 0, w, waterfallHeight);
    drawWaterfallGL();

    // Draw overlays (full viewport)
    glViewport(0, 0, w, h);
    drawOverlaysGL();
}

void MiniPanWidget::drawWaterfallGL() {
    if (!m_waterfallShader || m_smoothedSpectrum.isEmpty())
        return;

    m_waterfallShader->bind();

    // Bind waterfall texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_waterfallTexture);
    m_waterfallShader->setUniformValue("waterfallTex", 0);

    // Bind color LUT texture
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

void MiniPanWidget::drawSpectrumGL(int spectrumHeight) {
    if (!m_spectrumShader || m_smoothedSpectrum.isEmpty())
        return;

    qreal dpr = devicePixelRatioF();
    int w = static_cast<int>(width() * dpr);

    // Build spectrum line vertices with peak-hold downsampling
    QVector<float> vertices;
    int dataSize = m_smoothedSpectrum.size();
    float scale = static_cast<float>(dataSize) / w;

    // Find smoothed baseline
    float frameMin = 1.0f;
    for (int x = 0; x < w; ++x) {
        int startBin = static_cast<int>(x * scale);
        int endBin = static_cast<int>((x + 1) * scale);
        startBin = qBound(0, startBin, dataSize - 1);
        endBin = qBound(startBin + 1, endBin, dataSize);

        float peak = m_smoothedSpectrum[startBin];
        for (int i = startBin + 1; i < endBin; ++i) {
            if (m_smoothedSpectrum[i] > peak) {
                peak = m_smoothedSpectrum[i];
            }
        }

        float normalized = normalizeDb(peak);
        if (normalized < frameMin) {
            frameMin = normalized;
        }
    }

    // Smooth the baseline
    const float baselineAlpha = 0.05f;
    if (m_smoothedBaseline < 0.001f) {
        m_smoothedBaseline = frameMin;
    } else {
        m_smoothedBaseline = baselineAlpha * frameMin + (1.0f - baselineAlpha) * m_smoothedBaseline;
    }

    // Build vertices
    for (int x = 0; x < w; ++x) {
        int startBin = static_cast<int>(x * scale);
        int endBin = static_cast<int>((x + 1) * scale);
        startBin = qBound(0, startBin, dataSize - 1);
        endBin = qBound(startBin + 1, endBin, dataSize);

        float peak = m_smoothedSpectrum[startBin];
        for (int i = startBin + 1; i < endBin; ++i) {
            if (m_smoothedSpectrum[i] > peak) {
                peak = m_smoothedSpectrum[i];
            }
        }

        float normalized = normalizeDb(peak);
        float adjusted = normalized - m_smoothedBaseline;
        float lineHeight = adjusted * spectrumHeight * 0.95f * m_heightBoost;
        float y = spectrumHeight - lineHeight;

        vertices.append(static_cast<float>(x));
        vertices.append(y);
    }

    // Upload vertices
    m_spectrumVBO.bind();
    m_spectrumVBO.allocate(vertices.data(), vertices.size() * sizeof(float));

    // Draw spectrum line
    m_spectrumShader->bind();
    m_spectrumShader->setUniformValue("viewportSize", QVector2D(w, spectrumHeight));
    m_spectrumShader->setUniformValue(
        "lineColor", QVector4D(m_spectrumColor.redF(), m_spectrumColor.greenF(), m_spectrumColor.blueF(), 1.0f));

    int posLoc = m_spectrumShader->attributeLocation("position");
    m_spectrumShader->enableAttributeArray(posLoc);
    m_spectrumShader->setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));

    glLineWidth(1.0f);
    glDrawArrays(GL_LINE_STRIP, 0, vertices.size() / 2);

    m_spectrumShader->disableAttributeArray(posLoc);
    m_spectrumShader->release();
}

void MiniPanWidget::drawOverlaysGL() {
    if (!m_overlayShader)
        return;

    qreal dpr = devicePixelRatioF();
    int w = static_cast<int>(width() * dpr);
    int h = static_cast<int>(height() * dpr);
    int spectrumHeight = static_cast<int>(h * m_spectrumRatio);

    m_overlayShader->bind();
    m_overlayShader->setUniformValue("viewportSize", QVector2D(w, h));

    int posLoc = m_overlayShader->attributeLocation("position");

    // Draw separator line between spectrum and waterfall
    QVector<float> separatorLine = {0, static_cast<float>(spectrumHeight), static_cast<float>(w),
                                    static_cast<float>(spectrumHeight)};
    m_overlayVBO.bind();
    m_overlayVBO.allocate(separatorLine.data(), separatorLine.size() * sizeof(float));
    m_overlayShader->setUniformValue("color", QVector4D(0.2f, 0.2f, 0.2f, 1.0f));
    m_overlayShader->enableAttributeArray(posLoc);
    m_overlayShader->setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));
    glLineWidth(1.0f);
    glDrawArrays(GL_LINES, 0, 2);

    // Draw filter passband
    if (m_filterBw > 0) {
        int centerX = w / 2;
        int bwPixels = (m_filterBw * w) / m_bandwidthHz;

        // Calculate shift offset for CW modes
        int shiftHz = m_ifShift * 10;
        int shiftOffsetHz = shiftHz - m_cwPitch;
        int shiftPixels = (shiftOffsetHz * w) / m_bandwidthHz;

        int passbandX;
        if (m_mode == "CW" || m_mode == "CW-R") {
            passbandX = centerX + shiftPixels - bwPixels / 2;
        } else if (m_mode == "LSB") {
            passbandX = centerX - bwPixels;
        } else {
            passbandX = centerX;
        }

        // Draw passband quad
        QColor fillColor = m_passbandColor;
        fillColor.setAlpha(64);
        QVector<float> passbandQuad = {static_cast<float>(passbandX),
                                       0,
                                       static_cast<float>(passbandX + bwPixels),
                                       0,
                                       static_cast<float>(passbandX + bwPixels),
                                       static_cast<float>(h),
                                       static_cast<float>(passbandX),
                                       0,
                                       static_cast<float>(passbandX + bwPixels),
                                       static_cast<float>(h),
                                       static_cast<float>(passbandX),
                                       static_cast<float>(h)};
        m_overlayVBO.allocate(passbandQuad.data(), passbandQuad.size() * sizeof(float));
        m_overlayShader->setUniformValue(
            "color", QVector4D(fillColor.redF(), fillColor.greenF(), fillColor.blueF(), fillColor.alphaF()));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // Draw frequency marker (center line)
    int centerX = w / 2;
    QColor markerColor = m_passbandColor;
    markerColor.setAlpha(255);
    markerColor = markerColor.darker(150);

    QVector<float> markerLine = {static_cast<float>(centerX), 0, static_cast<float>(centerX), static_cast<float>(h)};
    m_overlayVBO.allocate(markerLine.data(), markerLine.size() * sizeof(float));
    m_overlayShader->setUniformValue("color",
                                     QVector4D(markerColor.redF(), markerColor.greenF(), markerColor.blueF(), 1.0f));
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 2);

    // Draw notch filter marker
    if (m_notchEnabled && m_notchPitchHz > 0) {
        int offsetHz;
        if (m_mode == "LSB") {
            offsetHz = -m_notchPitchHz;
        } else {
            offsetHz = m_notchPitchHz;
        }

        int notchX = centerX + (offsetHz * w) / m_bandwidthHz;

        if (notchX >= 0 && notchX < w) {
            QVector<float> notchLine = {static_cast<float>(notchX), 0, static_cast<float>(notchX),
                                        static_cast<float>(h)};
            m_overlayVBO.allocate(notchLine.data(), notchLine.size() * sizeof(float));
            m_overlayShader->setUniformValue("color", QVector4D(1.0f, 0.0f, 0.0f, 1.0f)); // Red
            glLineWidth(2.0f);
            glDrawArrays(GL_LINES, 0, 2);
        }
    }

    // Draw border
    QVector<float> border = {0,
                             0,
                             static_cast<float>(w - 1),
                             0,
                             static_cast<float>(w - 1),
                             0,
                             static_cast<float>(w - 1),
                             static_cast<float>(h - 1),
                             static_cast<float>(w - 1),
                             static_cast<float>(h - 1),
                             0,
                             static_cast<float>(h - 1),
                             0,
                             static_cast<float>(h - 1),
                             0,
                             0};
    m_overlayVBO.allocate(border.data(), border.size() * sizeof(float));
    m_overlayShader->setUniformValue("color", QVector4D(0.27f, 0.27f, 0.27f, 1.0f)); // #444444
    glLineWidth(1.0f);
    glDrawArrays(GL_LINES, 0, 8);

    m_overlayShader->disableAttributeArray(posLoc);
    m_overlayShader->release();
}

void MiniPanWidget::updateSpectrum(const QByteArray &bins) {
    if (bins.isEmpty())
        return;

    // Decompress MiniPAN data: byte / 10 (different from main panadapter)
    m_spectrum.resize(bins.size());
    for (int i = 0; i < bins.size(); ++i) {
        m_spectrum[i] = static_cast<quint8>(bins[i]) / 10.0f;
    }

    // Skip bins at edges to remove blank areas
    const int SKIP_START = 75;
    const int SKIP_END = 75;

    if (m_spectrum.size() > SKIP_START + SKIP_END + 10) {
        int validSize = m_spectrum.size() - SKIP_START - SKIP_END;
        QVector<float> trimmed(validSize);
        for (int i = 0; i < validSize; ++i) {
            trimmed[i] = m_spectrum[SKIP_START + i];
        }
        m_spectrum = trimmed;
    }

    // Apply smoothing
    if (m_smoothedSpectrum.size() != m_spectrum.size()) {
        m_smoothedSpectrum = m_spectrum;
    } else {
        for (int i = 0; i < m_spectrum.size(); ++i) {
            m_smoothedSpectrum[i] =
                m_smoothingAlpha * m_spectrum[i] + (1.0f - m_smoothingAlpha) * m_smoothedSpectrum[i];
        }
    }

    // Update waterfall texture
    if (m_glInitialized && !m_smoothedSpectrum.isEmpty()) {
        makeCurrent();

        // Resample spectrum to texture width
        QVector<GLubyte> row(TEXTURE_WIDTH);
        int dataSize = m_smoothedSpectrum.size();
        for (int i = 0; i < TEXTURE_WIDTH; ++i) {
            float srcPos = static_cast<float>(i) / (TEXTURE_WIDTH - 1) * (dataSize - 1);
            int idx = qBound(0, qRound(srcPos), dataSize - 1);

            // Average downsampling for waterfall
            int startBin = static_cast<int>(static_cast<float>(i) / TEXTURE_WIDTH * dataSize);
            int endBin = static_cast<int>(static_cast<float>(i + 1) / TEXTURE_WIDTH * dataSize);
            startBin = qBound(0, startBin, dataSize - 1);
            endBin = qBound(startBin + 1, endBin, dataSize);

            float sum = 0.0f;
            int count = endBin - startBin;
            for (int j = startBin; j < endBin; ++j) {
                sum += m_smoothedSpectrum[j];
            }
            float avgDb = sum / count;

            float normalized = normalizeDb(avgDb);
            row[i] = static_cast<GLubyte>(qBound(0, static_cast<int>(normalized * 255), 255));
        }

        // Upload single row to texture
        glBindTexture(GL_TEXTURE_2D, m_waterfallTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, m_waterfallWriteRow, TEXTURE_WIDTH, 1, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                        row.data());

        m_waterfallWriteRow = (m_waterfallWriteRow + 1) % WATERFALL_HISTORY;

        doneCurrent();
    }

    update();
}

void MiniPanWidget::clear() {
    m_spectrum.clear();
    m_smoothedSpectrum.clear();
    m_waterfallWriteRow = 0;
    m_smoothedBaseline = 0.0f;

    if (m_glInitialized) {
        makeCurrent();
        QVector<GLubyte> zeros(TEXTURE_WIDTH * WATERFALL_HISTORY, 0);
        glBindTexture(GL_TEXTURE_2D, m_waterfallTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TEXTURE_WIDTH, WATERFALL_HISTORY, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                        zeros.data());
        doneCurrent();
    }

    update();
}

float MiniPanWidget::normalizeDb(float db) {
    return qBound(0.0f, (db - m_minDb) / (m_maxDb - m_minDb), 1.0f);
}

int MiniPanWidget::bandwidthForMode(const QString &mode) const {
    if (mode == "CW" || mode == "CW-R") {
        return 3000;
    }
    return 10000;
}

void MiniPanWidget::setSpectrumColor(const QColor &color) {
    if (m_spectrumColor != color) {
        m_spectrumColor = color;
        update();
    }
}

void MiniPanWidget::setPassbandColor(const QColor &color) {
    if (m_passbandColor != color) {
        m_passbandColor = color;
        update();
    }
}

void MiniPanWidget::setNotchFilter(bool enabled, int pitchHz) {
    if (m_notchEnabled != enabled || m_notchPitchHz != pitchHz) {
        m_notchEnabled = enabled;
        m_notchPitchHz = pitchHz;
        update();
    }
}

void MiniPanWidget::setMode(const QString &mode) {
    if (m_mode != mode) {
        m_mode = mode;
        m_bandwidthHz = bandwidthForMode(mode);
        update();
    }
}

void MiniPanWidget::setFilterBandwidth(int bwHz) {
    if (m_filterBw != bwHz) {
        m_filterBw = bwHz;
        update();
    }
}

void MiniPanWidget::setIfShift(int shift) {
    if (m_ifShift != shift) {
        m_ifShift = shift;
        update();
    }
}

void MiniPanWidget::setCwPitch(int pitchHz) {
    if (m_cwPitch != pitchHz) {
        m_cwPitch = pitchHz;
        update();
    }
}

void MiniPanWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked();
        event->accept();
    } else {
        QOpenGLWidget::mousePressEvent(event);
    }
}
