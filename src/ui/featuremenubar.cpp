#include "featuremenubar.h"
#include <QHBoxLayout>
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include <QHideEvent>
#include <QKeyEvent>

FeatureMenuBar::FeatureMenuBar(QWidget *parent) : QWidget(parent) {
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setFocusPolicy(Qt::StrongFocus);
    setupUi();
    hide(); // Hidden by default
}

void FeatureMenuBar::setupUi() {
    setFixedHeight(52);

    auto *layout = new QHBoxLayout(this);
    // Symmetric margins for centered popup
    layout->setContentsMargins(12, 6, 12, 6);
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

    // Layout - compact, no stretches (popup is centered by showAboveWidget)
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_toggleBtn);
    layout->addWidget(m_extraBtn);
    layout->addWidget(m_valueLabel);
    layout->addWidget(m_decrementBtn);
    layout->addWidget(m_incrementBtn);

    // Connect signals
    connect(m_toggleBtn, &QPushButton::clicked, this, &FeatureMenuBar::toggleRequested);
    connect(m_decrementBtn, &QPushButton::clicked, this, &FeatureMenuBar::decrementRequested);
    connect(m_incrementBtn, &QPushButton::clicked, this, &FeatureMenuBar::incrementRequested);
    connect(m_extraBtn, &QPushButton::clicked, this, &FeatureMenuBar::extraButtonClicked);
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
    adjustSize(); // Calculate proper size for content

    // Position above reference widget if set
    if (m_referenceWidget) {
        showAboveWidget(m_referenceWidget);
    } else {
        update();
        show();
        setFocus();
    }
}

void FeatureMenuBar::showAboveWidget(QWidget *referenceWidget) {
    if (!referenceWidget)
        return;

    m_referenceWidget = referenceWidget;

    // Force size calculation
    layout()->activate();
    adjustSize();

    // Get the reference widget's global position
    QPoint refGlobal = referenceWidget->mapToGlobal(QPoint(0, 0));
    int refCenterX = refGlobal.x() + referenceWidget->width() / 2;

    // Center popup horizontally above reference widget
    int popupX = refCenterX - width() / 2;
    int popupY = refGlobal.y() - height() - 4; // 4px gap above

    // Ensure popup stays on screen
    QRect screenGeom = QApplication::primaryScreen()->availableGeometry();
    if (popupX < screenGeom.left()) {
        popupX = screenGeom.left();
    } else if (popupX + width() > screenGeom.right()) {
        popupX = screenGeom.right() - width();
    }
    if (popupY < screenGeom.top()) {
        // If not enough room above, show below instead
        popupY = refGlobal.y() + referenceWidget->height() + 4;
    }

    move(popupX, popupY);
    show();
    setFocus();
    update();
}

void FeatureMenuBar::hideMenu() {
    hide();
    // closed() signal is emitted by hideEvent()
}

void FeatureMenuBar::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    emit closed();
}

void FeatureMenuBar::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        hideMenu();
    } else {
        QWidget::keyPressEvent(event);
    }
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

void FeatureMenuBar::setNbFilter(int filter) {
    m_nbFilter = qBound(0, filter, 2);
    // Update extra button text to show current filter
    static const char *filterNames[] = {"FILTER\nNONE", "FILTER\nNARROW", "FILTER\nWIDE"};
    m_extraBtn->setText(filterNames[m_nbFilter]);
}

void FeatureMenuBar::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Calculate tight bounding box from first to last visible widget
    int left = m_titleLabel->geometry().left() - 8; // 8px padding
    int right = m_incrementBtn->geometry().right() + 8;
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
    drawDelimiter(m_valueLabel); // After value
    // No delimiter after +/- buttons (they're at the end now)
}
