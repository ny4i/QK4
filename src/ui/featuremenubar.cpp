#include "featuremenubar.h"
#include <QHBoxLayout>
#include <QPainter>

FeatureMenuBar::FeatureMenuBar(QWidget *parent) : QWidget(parent) {
    setupUi();
    hide(); // Hidden by default
}

void FeatureMenuBar::setupUi() {
    setFixedHeight(52);

    auto *layout = new QHBoxLayout(this);
    // Left margin matches side panel width to align with bottom menu bar
    layout->setContentsMargins(105, 6, 10, 6);
    layout->setSpacing(8);

    // Title label (framed box with centered text)
    m_titleLabel = new QLabel("ATTENUATOR", this);
    m_titleLabel->setFixedSize(140, 36);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("QLabel {"
                                "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                                "    stop:0 #4a4a4a, stop:0.4 #3a3a3a, stop:0.6 #353535, stop:1 #2a2a2a);"
                                "  color: #FFFFFF;"
                                "  border: 1px solid #606060;"
                                "  border-radius: 5px;"
                                "  font-size: 14px;"
                                "  font-weight: bold;"
                                "}");

    // OFF/ON toggle button
    m_toggleBtn = new QPushButton("OFF", this);
    m_toggleBtn->setMinimumWidth(60);
    m_toggleBtn->setFixedHeight(36);
    m_toggleBtn->setCursor(Qt::PointingHandCursor);
    m_toggleBtn->setStyleSheet(buttonStyle());

    // Extra button (only shown for NB LEVEL - "FILTER NONE")
    m_extraBtn = new QPushButton("FILTER\nNONE", this);
    m_extraBtn->setMinimumWidth(70);
    m_extraBtn->setFixedHeight(36);
    m_extraBtn->setCursor(Qt::PointingHandCursor);
    m_extraBtn->setStyleSheet(buttonStyle());
    m_extraBtn->hide(); // Hidden by default

    // Value label (center, bold)
    m_valueLabel = new QLabel("0", this);
    m_valueLabel->setStyleSheet("color: #FFFFFF; font-size: 14px; font-weight: bold;");
    m_valueLabel->setAlignment(Qt::AlignCenter);
    m_valueLabel->setMinimumWidth(80);

    // Decrement button
    m_decrementBtn = new QPushButton("-", this);
    m_decrementBtn->setFixedSize(44, 36);
    m_decrementBtn->setCursor(Qt::PointingHandCursor);
    m_decrementBtn->setStyleSheet(buttonStyleSmall());

    // Increment button
    m_incrementBtn = new QPushButton("+", this);
    m_incrementBtn->setFixedSize(44, 36);
    m_incrementBtn->setCursor(Qt::PointingHandCursor);
    m_incrementBtn->setStyleSheet(buttonStyleSmall());

    // Encoder icon (⟳) - static indicator
    m_encoderIcon = new QLabel(this);
    m_encoderIcon->setFixedSize(36, 36);
    m_encoderIcon->setAlignment(Qt::AlignCenter);
    m_encoderIcon->setStyleSheet("QLabel {"
                                 "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                                 "    stop:0 #4a4a4a, stop:0.4 #3a3a3a, stop:0.6 #353535, stop:1 #2a2a2a);"
                                 "  color: #FFFFFF;"
                                 "  border: 1px solid #606060;"
                                 "  border-radius: 5px;"
                                 "  font-size: 16px;"
                                 "}");
    // Unicode for refresh/cycle symbol
    m_encoderIcon->setText("\u21BB"); // ↻ or could use ⟳ (U+27F3)

    // Encoder label [A] - static indicator (boxed)
    m_encoderLabel = new QLabel("A", this);
    m_encoderLabel->setFixedSize(36, 36);
    m_encoderLabel->setAlignment(Qt::AlignCenter);
    m_encoderLabel->setStyleSheet("QLabel {"
                                  "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                                  "    stop:0 #4a4a4a, stop:0.4 #3a3a3a, stop:0.6 #353535, stop:1 #2a2a2a);"
                                  "  color: #FFFFFF;"
                                  "  border: 1px solid #606060;"
                                  "  border-radius: 5px;"
                                  "  font-size: 14px;"
                                  "  font-weight: bold;"
                                  "}");

    // Close/back button (←)
    m_closeBtn = new QPushButton("\u2190", this); // ← arrow
    m_closeBtn->setFixedSize(44, 36);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setStyleSheet(buttonStyleSmall());

    // Layout - center content within the menu bar
    layout->addStretch(); // Left stretch for centering
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_toggleBtn);
    layout->addWidget(m_extraBtn);
    layout->addWidget(m_valueLabel);
    layout->addWidget(m_decrementBtn);
    layout->addWidget(m_incrementBtn);
    layout->addWidget(m_encoderIcon);
    layout->addWidget(m_encoderLabel);
    layout->addWidget(m_closeBtn);
    layout->addStretch(); // Right stretch for centering

    // Connect signals
    connect(m_toggleBtn, &QPushButton::clicked, this, &FeatureMenuBar::toggleRequested);
    connect(m_decrementBtn, &QPushButton::clicked, this, &FeatureMenuBar::decrementRequested);
    connect(m_incrementBtn, &QPushButton::clicked, this, &FeatureMenuBar::incrementRequested);
    connect(m_extraBtn, &QPushButton::clicked, this, &FeatureMenuBar::extraButtonClicked);
    connect(m_closeBtn, &QPushButton::clicked, this, &FeatureMenuBar::closeRequested);
}

QString FeatureMenuBar::buttonStyle() const {
    return R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4a4a4a, stop:0.4 #3a3a3a, stop:0.6 #353535, stop:1 #2a2a2a);
            color: #FFFFFF;
            border: 1px solid #606060;
            border-radius: 5px;
            padding: 6px 12px;
            font-size: 12px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #5a5a5a, stop:0.4 #4a4a4a, stop:0.6 #454545, stop:1 #3a3a3a);
            border: 1px solid #808080;
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2a2a2a, stop:0.4 #353535, stop:0.6 #3a3a3a, stop:1 #4a4a4a);
            border: 1px solid #909090;
        }
    )";
}

QString FeatureMenuBar::buttonStyleSmall() const {
    return R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4a4a4a, stop:0.4 #3a3a3a, stop:0.6 #353535, stop:1 #2a2a2a);
            color: #FFFFFF;
            border: 1px solid #606060;
            border-radius: 5px;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #5a5a5a, stop:0.4 #4a4a4a, stop:0.6 #454545, stop:1 #3a3a3a);
            border: 1px solid #808080;
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2a2a2a, stop:0.4 #353535, stop:0.6 #3a3a3a, stop:1 #4a4a4a);
            border: 1px solid #909090;
        }
    )";
}

void FeatureMenuBar::showForFeature(Feature feature) {
    m_currentFeature = feature;
    updateForFeature();
    // Force layout recalculation before showing (extra button changes width)
    layout()->activate();
    update(); // Repaint with new geometry
    show();
}

void FeatureMenuBar::hideMenu() {
    hide();
}

void FeatureMenuBar::updateForFeature() {
    switch (m_currentFeature) {
    case Attenuator:
        m_titleLabel->setText("ATTENUATOR");
        m_extraBtn->hide();
        m_valueUnit = " dB";
        break;
    case NbLevel:
        m_titleLabel->setText("NB LEVEL");
        m_extraBtn->setText("FILTER\nNONE");
        m_extraBtn->show();
        m_valueUnit = "";
        break;
    case NrAdjust:
        m_titleLabel->setText("NR ADJUST");
        m_extraBtn->hide();
        m_valueUnit = "";
        break;
    case ManualNotch:
        m_titleLabel->setText("MANUAL NOTCH");
        m_extraBtn->hide();
        m_valueUnit = " Hz";
        break;
    }

    // Update value display with unit
    setValue(m_value);
}

void FeatureMenuBar::setFeatureEnabled(bool enabled) {
    m_featureEnabled = enabled;
    m_toggleBtn->setText(enabled ? "ON" : "OFF");
}

void FeatureMenuBar::setValue(int value) {
    m_value = value;
    m_valueLabel->setText(QString::number(value) + m_valueUnit);
}

void FeatureMenuBar::setValueUnit(const QString &unit) {
    m_valueUnit = unit;
    setValue(m_value); // Refresh display
}

void FeatureMenuBar::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Calculate tight bounding box from first to last visible widget
    int left = m_titleLabel->geometry().left() - 8;   // 8px padding
    int right = m_closeBtn->geometry().right() + 8;
    QRect contentRect(left, 1, right - left, height() - 3);

    // Gradient background (matches ControlGroupWidget style)
    QLinearGradient grad(0, 0, 0, height());
    grad.setColorAt(0, QColor(74, 74, 74));
    grad.setColorAt(0.4, QColor(58, 58, 58));
    grad.setColorAt(0.6, QColor(53, 53, 53));
    grad.setColorAt(1, QColor(42, 42, 42));

    // Draw rounded rectangle with border around content area only
    painter.setBrush(grad);
    painter.setPen(QPen(QColor(96, 96, 96), 1)); // Light grey border
    painter.drawRoundedRect(contentRect, 8, 8);

    // Draw vertical delimiter lines between widget groups
    painter.setPen(QPen(QColor(96, 96, 96), 1));
    int lineTop = 8;
    int lineBottom = height() - 8;

    // Helper to draw delimiter after a widget
    auto drawDelimiter = [&](QWidget *widget) {
        if (widget && widget->isVisible()) {
            int x = widget->geometry().right() + 4; // 4px after widget (half of spacing)
            painter.drawLine(x, lineTop, x, lineBottom);
        }
    };

    // Draw delimiters after each section
    drawDelimiter(m_titleLabel); // After title
    drawDelimiter(m_toggleBtn);  // After OFF/ON
    if (m_extraBtn->isVisible()) {
        drawDelimiter(m_extraBtn); // After FILTER NONE (if shown)
    }
    drawDelimiter(m_valueLabel);   // After value
    drawDelimiter(m_incrementBtn); // After +/- buttons (group them)
    drawDelimiter(m_encoderLabel); // After encoder hints (group them)
}
