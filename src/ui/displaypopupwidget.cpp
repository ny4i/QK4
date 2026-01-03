#include "displaypopupwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QHideEvent>
#include <QApplication>
#include <QScreen>

namespace {
// Indicator bar/triangle color
const QColor IndicatorColor(85, 85, 85); // #555555

// Layout constants
const int MenuButtonWidth = 80;
const int MenuButtonHeight = 44;
const int TopRowHeight = 36;
const int ButtonSpacing = 4;
const int RowSpacing = 4;
const int Margin = 8;
const int TriangleWidth = 24;
const int TriangleHeight = 12;
const int BottomStripHeight = 8;

// Colors
const char *AmberColor = "#FFB000";
} // namespace

// ============================================================================
// DisplayMenuButton Implementation
// ============================================================================

DisplayMenuButton::DisplayMenuButton(const QString &primaryText, const QString &alternateText, QWidget *parent)
    : QWidget(parent), m_primaryText(primaryText), m_alternateText(alternateText) {
    setFixedSize(MenuButtonWidth, MenuButtonHeight);
    setCursor(Qt::PointingHandCursor);
}

void DisplayMenuButton::setSelected(bool selected) {
    if (m_selected != selected) {
        m_selected = selected;
        update();
    }
}

void DisplayMenuButton::setPrimaryText(const QString &text) {
    if (m_primaryText != text) {
        m_primaryText = text;
        update();
    }
}

void DisplayMenuButton::setAlternateText(const QString &text) {
    if (m_alternateText != text) {
        m_alternateText = text;
        update();
    }
}

void DisplayMenuButton::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background - subtle gradient
    QLinearGradient grad(0, 0, 0, height());
    if (m_hovered) {
        grad.setColorAt(0, QColor(90, 90, 90));
        grad.setColorAt(0.4, QColor(74, 74, 74));
        grad.setColorAt(0.6, QColor(69, 69, 69));
        grad.setColorAt(1, QColor(58, 58, 58));
    } else {
        grad.setColorAt(0, QColor(74, 74, 74));
        grad.setColorAt(0.4, QColor(58, 58, 58));
        grad.setColorAt(0.6, QColor(53, 53, 53));
        grad.setColorAt(1, QColor(42, 42, 42));
    }
    painter.setBrush(grad);
    painter.setPen(QPen(QColor(96, 96, 96), 1));
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 5, 5);

    // Primary text (white) - top
    QFont primaryFont = font();
    primaryFont.setPixelSize(12);
    primaryFont.setBold(m_selected);
    painter.setFont(primaryFont);
    painter.setPen(Qt::white);

    QRect primaryRect(0, 4, width(), height() / 2 - 2);
    painter.drawText(primaryRect, Qt::AlignCenter, m_primaryText);

    // Alternate text (orange) - bottom
    QFont altFont = font();
    altFont.setPixelSize(10);
    altFont.setBold(false);
    painter.setFont(altFont);
    painter.setPen(QColor(AmberColor));

    QRect altRect(0, height() / 2, width(), height() / 2 - 4);
    painter.drawText(altRect, Qt::AlignCenter, m_alternateText);
}

void DisplayMenuButton::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    } else if (event->button() == Qt::RightButton) {
        emit rightClicked();
    }
}

void DisplayMenuButton::enterEvent(QEnterEvent *event) {
    Q_UNUSED(event)
    m_hovered = true;
    update();
}

void DisplayMenuButton::leaveEvent(QEvent *event) {
    Q_UNUSED(event)
    m_hovered = false;
    update();
}

// ============================================================================
// ControlGroupWidget Implementation
// ============================================================================

namespace {
const int ControlGroupHeight = 32;
} // namespace

ControlGroupWidget::ControlGroupWidget(const QString &label, QWidget *parent)
    : QWidget(parent), m_label(label), m_value("100.0") {
    // Fixed width for control group: label + value + - + + with generous spacing
    setFixedSize(180, ControlGroupHeight);
    setCursor(Qt::PointingHandCursor);
}

void ControlGroupWidget::setValue(const QString &value) {
    if (m_value != value) {
        m_value = value;
        update();
    }
}

void ControlGroupWidget::setShowAutoButton(bool show) {
    if (m_showAutoButton != show) {
        m_showAutoButton = show;
        // Resize widget to accommodate AUTO button
        if (show) {
            setFixedSize(220, ControlGroupHeight);
        } else {
            setFixedSize(180, ControlGroupHeight);
        }
        update();
    }
}

void ControlGroupWidget::setAutoEnabled(bool enabled) {
    if (m_autoEnabled != enabled) {
        m_autoEnabled = enabled;
        update();
    }
}

void ControlGroupWidget::setValueFaded(bool faded) {
    if (m_valueFaded != faded) {
        m_valueFaded = faded;
        update();
    }
}

void ControlGroupWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Layout depends on whether AUTO button is shown
    const int labelWidth = 60; // Fits "AVERAGE" label
    const int autoWidth = m_showAutoButton ? 40 : 0;
    const int valueWidth = 52;
    const int buttonWidth = 32;

    // Draw container background with gradient
    QLinearGradient grad(0, 0, 0, height());
    grad.setColorAt(0, QColor(74, 74, 74));
    grad.setColorAt(0.4, QColor(58, 58, 58));
    grad.setColorAt(0.6, QColor(53, 53, 53));
    grad.setColorAt(1, QColor(42, 42, 42));

    // Rounded rectangle with softer corners
    int cornerRadius = 6;
    painter.setBrush(grad);
    painter.setPen(QPen(QColor(96, 96, 96), 1));
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), cornerRadius, cornerRadius);

    // Calculate positions
    int x = 4;
    QRect labelRect(x, 2, labelWidth, height() - 4);
    x += labelWidth;

    // Vertical line after label
    painter.drawLine(x, 4, x, height() - 4);

    // AUTO button (if shown)
    if (m_showAutoButton) {
        m_autoRect = QRect(x, 2, autoWidth, height() - 4);
        x += autoWidth;

        // Vertical line after AUTO
        painter.drawLine(x, 4, x, height() - 4);
    }

    QRect valueRect(x, 2, valueWidth, height() - 4);
    x += valueWidth;

    // Vertical line after value
    painter.drawLine(x, 4, x, height() - 4);

    m_minusRect = QRect(x, 2, buttonWidth, height() - 4);
    x += buttonWidth;

    // Vertical line after minus
    painter.drawLine(x, 4, x, height() - 4);

    m_plusRect = QRect(x, 2, buttonWidth, height() - 4);

    // Draw label
    QFont labelFont = font();
    labelFont.setPixelSize(11);
    labelFont.setBold(true);
    painter.setFont(labelFont);
    painter.setPen(Qt::white);
    painter.drawText(labelRect, Qt::AlignCenter, m_label);

    // Draw AUTO button if shown
    if (m_showAutoButton) {
        if (m_autoEnabled) {
            painter.fillRect(m_autoRect, Qt::white);
            painter.setPen(QColor(0x66, 0x66, 0x66));
        } else {
            painter.setPen(Qt::white);
        }
        QFont autoFont = font();
        autoFont.setPixelSize(10);
        autoFont.setBold(true);
        painter.setFont(autoFont);
        painter.drawText(m_autoRect, Qt::AlignCenter, "AUTO");
        painter.setFont(labelFont);
        painter.setPen(Qt::white);
    }

    // Draw value (faded grey when auto mode)
    if (m_valueFaded) {
        painter.setPen(QColor(128, 128, 128)); // Faded grey
    } else {
        painter.setPen(Qt::white);
    }
    painter.drawText(valueRect, Qt::AlignCenter, m_value);
    painter.setPen(Qt::white); // Reset for buttons

    // Draw minus button with larger font
    QFont buttonFont = font();
    buttonFont.setPixelSize(16);
    buttonFont.setBold(true);
    painter.setFont(buttonFont);
    painter.drawText(m_minusRect, Qt::AlignCenter, "-");

    // Draw plus button
    painter.drawText(m_plusRect, Qt::AlignCenter, "+");
}

void ControlGroupWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        QPoint pos = event->pos();
        if (m_showAutoButton && m_autoRect.contains(pos)) {
            emit autoClicked();
        } else if (m_minusRect.contains(pos)) {
            emit decrementClicked();
        } else if (m_plusRect.contains(pos)) {
            emit incrementClicked();
        }
    }
}

// ============================================================================
// ToggleGroupWidget Implementation
// ============================================================================

namespace {
const int ToggleGroupHeight = 32;
const int TogglePadding = 6;
const int AmpWidth = 32;            // Generous width for clickable & area
const int ToggleTriangleWidth = 10; // Triangle pointing to next element
} // namespace

ToggleGroupWidget::ToggleGroupWidget(const QString &leftLabel, const QString &rightLabel, QWidget *parent)
    : QWidget(parent), m_leftLabel(leftLabel), m_rightLabel(rightLabel) {
    // Calculate width based on labels - generous spacing for readability
    QFontMetrics fm(font());
    int leftWidth = fm.horizontalAdvance(leftLabel) + 24;
    int rightWidth = fm.horizontalAdvance(rightLabel) + 24;
    int totalWidth = leftWidth + AmpWidth + rightWidth + TogglePadding * 2 + ToggleTriangleWidth;

    setFixedSize(totalWidth, ToggleGroupHeight);
    setCursor(Qt::PointingHandCursor);
}

void ToggleGroupWidget::setLeftSelected(bool selected) {
    if (m_leftSelected != selected) {
        m_leftSelected = selected;
        update();
    }
}

void ToggleGroupWidget::setRightSelected(bool selected) {
    if (m_rightSelected != selected) {
        m_rightSelected = selected;
        update();
    }
}

void ToggleGroupWidget::setRightEnabled(bool enabled) {
    if (m_rightEnabled != enabled) {
        m_rightEnabled = enabled;
        update();
    }
}

void ToggleGroupWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QFontMetrics fm(font());
    int leftWidth = fm.horizontalAdvance(m_leftLabel) + 24;
    int rightWidth = fm.horizontalAdvance(m_rightLabel) + 24;

    // Calculate zones (excluding triangle area on right)
    int mainWidth = width() - ToggleTriangleWidth;
    m_leftRect = QRect(TogglePadding, 2, leftWidth, height() - 4);
    int ampX = m_leftRect.right();
    m_ampRect = QRect(ampX, 2, AmpWidth, height() - 4);
    m_rightRect = QRect(ampX + AmpWidth, 2, rightWidth, height() - 4);

    // Draw container background with gradient
    QLinearGradient grad(0, 0, 0, height());
    grad.setColorAt(0, QColor(74, 74, 74));
    grad.setColorAt(0.4, QColor(58, 58, 58));
    grad.setColorAt(0.6, QColor(53, 53, 53));
    grad.setColorAt(1, QColor(42, 42, 42));

    // Create path with rounded left corners and triangle on right
    QPainterPath containerPath;
    int cornerRadius = 6;

    // Start from top-left corner
    containerPath.moveTo(cornerRadius, 0);
    // Top edge to where triangle starts
    containerPath.lineTo(mainWidth, 0);
    // Triangle pointing right
    containerPath.lineTo(mainWidth, height() / 2 - 6);
    containerPath.lineTo(width(), height() / 2);
    containerPath.lineTo(mainWidth, height() / 2 + 6);
    // Continue to bottom
    containerPath.lineTo(mainWidth, height());
    // Bottom edge
    containerPath.lineTo(cornerRadius, height());
    // Bottom-left rounded corner
    containerPath.quadTo(0, height(), 0, height() - cornerRadius);
    // Left edge
    containerPath.lineTo(0, cornerRadius);
    // Top-left rounded corner
    containerPath.quadTo(0, 0, cornerRadius, 0);
    containerPath.closeSubpath();

    painter.setBrush(grad);
    painter.setPen(QPen(QColor(96, 96, 96), 1));
    painter.drawPath(containerPath);

    // Draw left toggle area
    QFont labelFont = font();
    labelFont.setPixelSize(11);
    labelFont.setBold(true);
    painter.setFont(labelFont);

    if (m_leftSelected) {
        // Selected: white background, grey text
        painter.fillRect(m_leftRect, Qt::white);
        painter.setPen(QColor(0x66, 0x66, 0x66));
    } else {
        // Unselected: no fill, white text
        painter.setPen(Qt::white);
    }
    painter.drawText(m_leftRect, Qt::AlignCenter, m_leftLabel);

    // Draw vertical demarcation line before &
    painter.setPen(QPen(QColor(96, 96, 96), 1));
    painter.drawLine(ampX, 4, ampX, height() - 4);

    // Highlight & area if both are selected
    if (m_leftSelected && m_rightSelected) {
        painter.fillRect(m_ampRect, Qt::white);
    }

    // Draw & separator with larger font
    QFont ampFont = font();
    ampFont.setPixelSize(16);
    ampFont.setBold(true);
    painter.setFont(ampFont);
    if (m_leftSelected && m_rightSelected) {
        painter.setPen(QColor(0x66, 0x66, 0x66)); // Grey text on white bg
    } else {
        painter.setPen(QColor(0xAA, 0xAA, 0xAA)); // Light grey text
    }
    painter.drawText(m_ampRect, Qt::AlignCenter, "&");

    // Draw vertical demarcation line after &
    painter.setPen(QPen(QColor(96, 96, 96), 1));
    painter.drawLine(ampX + AmpWidth, 4, ampX + AmpWidth, height() - 4);

    // Draw right toggle area
    if (m_rightEnabled) {
        if (m_rightSelected) {
            // Selected: white background, grey text
            painter.fillRect(m_rightRect, Qt::white);
            painter.setPen(QColor(0x66, 0x66, 0x66));
        } else {
            // Unselected: no fill, white text
            painter.setPen(Qt::white);
        }
    } else {
        // Disabled: dim grey text
        painter.setPen(QColor(0x66, 0x66, 0x66));
    }
    labelFont.setPixelSize(11);
    painter.setFont(labelFont);
    painter.drawText(m_rightRect, Qt::AlignCenter, m_rightLabel);
}

void ToggleGroupWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        QPoint pos = event->pos();
        if (m_leftRect.contains(pos)) {
            emit leftClicked();
        } else if (m_ampRect.contains(pos) && m_rightEnabled) {
            emit bothClicked();
        } else if (m_rightRect.contains(pos) && m_rightEnabled) {
            emit rightClicked();
        }
    }
}

// ============================================================================
// DisplayPopupWidget Implementation
// ============================================================================

DisplayPopupWidget::DisplayPopupWidget(QWidget *parent) : QWidget(parent), m_triangleXOffset(0) {
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setFocusPolicy(Qt::StrongFocus);

    setupUi();
}

void DisplayPopupWidget::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(Margin, Margin, Margin, Margin + BottomStripHeight + TriangleHeight);
    mainLayout->setSpacing(RowSpacing);

    setupTopRow();
    setupBottomRow();

    // Calculate size: 7 buttons + spacing
    int totalWidth = 7 * MenuButtonWidth + 6 * ButtonSpacing + 2 * Margin;
    int totalHeight = TopRowHeight + MenuButtonHeight + RowSpacing + 2 * Margin + BottomStripHeight + TriangleHeight;
    setFixedSize(totalWidth, totalHeight);

    // Initial selection
    updateMenuButtonStyles();
    updateToggleStyles();
    updateMenuButtonLabels();
}

void DisplayPopupWidget::setupTopRow() {
    auto *topRow = new QHBoxLayout();
    topRow->setSpacing(8);

    // === Target Toggle Groups (left side) ===

    // LCD & EXT toggle group
    m_lcdExtGroup = new ToggleGroupWidget("LCD", "EXT", this);
    m_lcdExtGroup->setLeftSelected(m_lcdEnabled);
    m_lcdExtGroup->setRightSelected(m_extEnabled);
    connect(m_lcdExtGroup, &ToggleGroupWidget::leftClicked, this, [this]() {
        m_lcdEnabled = true;
        m_extEnabled = false;
        m_lcdExtGroup->setLeftSelected(true);
        m_lcdExtGroup->setRightSelected(false);
        emit lcdToggled(true);
        emit extToggled(false);
    });
    connect(m_lcdExtGroup, &ToggleGroupWidget::rightClicked, this, [this]() {
        m_lcdEnabled = false;
        m_extEnabled = true;
        m_lcdExtGroup->setLeftSelected(false);
        m_lcdExtGroup->setRightSelected(true);
        emit lcdToggled(false);
        emit extToggled(true);
    });
    connect(m_lcdExtGroup, &ToggleGroupWidget::bothClicked, this, [this]() {
        m_lcdEnabled = true;
        m_extEnabled = true;
        m_lcdExtGroup->setLeftSelected(true);
        m_lcdExtGroup->setRightSelected(true);
        emit lcdToggled(true);
        emit extToggled(true);
    });
    topRow->addWidget(m_lcdExtGroup);

    // A & B toggle group
    m_vfoAbGroup = new ToggleGroupWidget("A", "B", this);
    m_vfoAbGroup->setLeftSelected(m_vfoAEnabled);
    m_vfoAbGroup->setRightSelected(m_vfoBEnabled);
    m_vfoAbGroup->setRightEnabled(m_vfoBAvailable);
    connect(m_vfoAbGroup, &ToggleGroupWidget::leftClicked, this, [this]() {
        m_vfoAEnabled = true;
        m_vfoBEnabled = false;
        m_vfoAbGroup->setLeftSelected(true);
        m_vfoAbGroup->setRightSelected(false);
        updateRefLevelControlGroup(); // Sync ref level display
        updateSpanControlGroup();     // Sync span display
        emit vfoAToggled(true);
        emit vfoBToggled(false);
    });
    connect(m_vfoAbGroup, &ToggleGroupWidget::rightClicked, this, [this]() {
        if (m_vfoBAvailable) {
            m_vfoAEnabled = false;
            m_vfoBEnabled = true;
            m_vfoAbGroup->setLeftSelected(false);
            m_vfoAbGroup->setRightSelected(true);
            updateRefLevelControlGroup(); // Sync ref level display
            updateSpanControlGroup();     // Sync span display
            emit vfoAToggled(false);
            emit vfoBToggled(true);
        }
    });
    connect(m_vfoAbGroup, &ToggleGroupWidget::bothClicked, this, [this]() {
        if (m_vfoBAvailable) {
            m_vfoAEnabled = true;
            m_vfoBEnabled = true;
            m_vfoAbGroup->setLeftSelected(true);
            m_vfoAbGroup->setRightSelected(true);
            updateRefLevelControlGroup(); // Sync ref level display
            updateSpanControlGroup();     // Sync span display
            emit vfoAToggled(true);
            emit vfoBToggled(true);
        }
    });
    topRow->addWidget(m_vfoAbGroup);

    topRow->addSpacing(8);

    // === Context-Dependent Control Area (right side) ===
    m_controlStack = new QStackedWidget(this);

    m_spanControlPage = createSpanControlPage();
    m_refLevelControlPage = createRefLevelControlPage();
    m_averageControlPage = createAverageControlPage();
    m_nbControlPage = createNbControlPage();
    m_defaultControlPage = createDefaultControlPage();

    m_controlStack->addWidget(m_spanControlPage);
    m_controlStack->addWidget(m_refLevelControlPage);
    m_controlStack->addWidget(m_averageControlPage);
    m_controlStack->addWidget(m_nbControlPage);
    m_controlStack->addWidget(m_defaultControlPage);

    // Default to SPAN page
    m_controlStack->setCurrentWidget(m_spanControlPage);

    topRow->addWidget(m_controlStack);
    topRow->addStretch();

    // Add top row to main layout
    static_cast<QVBoxLayout *>(layout())->addLayout(topRow);
}

QWidget *DisplayPopupWidget::createSpanControlPage() {
    auto *page = new QWidget(this);
    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_spanControlGroup = new ControlGroupWidget("SPAN", page);
    m_spanControlGroup->setValue("100.0");
    connect(m_spanControlGroup, &ControlGroupWidget::decrementClicked, this,
            &DisplayPopupWidget::spanDecrementRequested);
    connect(m_spanControlGroup, &ControlGroupWidget::incrementClicked, this,
            &DisplayPopupWidget::spanIncrementRequested);
    layout->addWidget(m_spanControlGroup);

    return page;
}

QWidget *DisplayPopupWidget::createRefLevelControlPage() {
    auto *page = new QWidget(this);
    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_refLevelControlGroup = new ControlGroupWidget("REF", page);
    m_refLevelControlGroup->setShowAutoButton(true);
    m_refLevelControlGroup->setValue("-108");
    connect(m_refLevelControlGroup, &ControlGroupWidget::decrementClicked, this, [this]() {
        bool useB = m_vfoBEnabled && !m_vfoAEnabled;
        QString suffix = useB ? "$" : "";
        emit catCommandRequested(QString("#REF%1-;").arg(suffix)); // Decrement ref level
        emit refLevelDecrementRequested();
    });
    connect(m_refLevelControlGroup, &ControlGroupWidget::incrementClicked, this, [this]() {
        bool useB = m_vfoBEnabled && !m_vfoAEnabled;
        QString suffix = useB ? "$" : "";
        emit catCommandRequested(QString("#REF%1+;").arg(suffix)); // Increment ref level
        emit refLevelIncrementRequested();
    });
    connect(m_refLevelControlGroup, &ControlGroupWidget::autoClicked, this, [this]() {
        // Auto-ref is GLOBAL - affects both VFOs, no suffix needed
        // K4 doesn't echo #AR commands, so use optimistic update
        emit catCommandRequested(QString("#AR/;")); // Toggle GLOBAL AUTO/MAN

        // Optimistic local update
        m_autoRef = !m_autoRef;
        updateRefLevelControlGroup();
        emit autoRefLevelToggled(m_autoRef);
    });
    layout->addWidget(m_refLevelControlGroup);

    // Sync initial AUTO button state with m_autoRef
    updateRefLevelControlGroup();

    return page;
}

QWidget *DisplayPopupWidget::createAverageControlPage() {
    auto *page = new QWidget(this);
    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_averageControlGroup = new ControlGroupWidget("AVERAGE", page);
    m_averageControlGroup->setValue(QString::number(m_averaging > 0 ? m_averaging : 5));
    connect(m_averageControlGroup, &ControlGroupWidget::decrementClicked, this,
            &DisplayPopupWidget::averagingDecrementRequested);
    connect(m_averageControlGroup, &ControlGroupWidget::incrementClicked, this,
            &DisplayPopupWidget::averagingIncrementRequested);
    layout->addWidget(m_averageControlGroup);

    return page;
}

QWidget *DisplayPopupWidget::createNbControlPage() {
    auto *page = new QWidget(this);
    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_nbControlGroup = new ControlGroupWidget("NB", page);
    updateNbControlGroupValue();
    connect(m_nbControlGroup, &ControlGroupWidget::decrementClicked, this,
            &DisplayPopupWidget::nbLevelDecrementRequested);
    connect(m_nbControlGroup, &ControlGroupWidget::incrementClicked, this,
            &DisplayPopupWidget::nbLevelIncrementRequested);
    layout->addWidget(m_nbControlGroup);

    return page;
}

void DisplayPopupWidget::updateNbControlGroupValue() {
    if (m_nbControlGroup) {
        QString modeText;
        switch (m_ddcNbMode) {
        case 0:
            modeText = "OFF";
            break;
        case 1:
            modeText = "ON";
            break;
        case 2:
            modeText = "AUTO";
            break;
        default:
            modeText = "OFF";
            break;
        }
        m_nbControlGroup->setValue(QString("%1  %2").arg(modeText).arg(m_ddcNbLevel));
    }
}

QWidget *DisplayPopupWidget::createDefaultControlPage() {
    auto *page = new QWidget(this);
    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    // Placeholder - empty for now
    auto *placeholder = new QLabel("", page);
    layout->addWidget(placeholder);

    return page;
}

void DisplayPopupWidget::setupBottomRow() {
    auto *bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(ButtonSpacing);

    // Menu items with primary/alternate labels
    struct MenuItemDef {
        QString primary;
        QString alternate;
        MenuItem item;
    };

    QList<MenuItemDef> items = {{"PAN = A", "WTRFALL", PanWaterfall}, {"NB", "WTR CLRS", NbWtrClrs},
                                {"REF LVL", "SCALE", RefLvlScale},    {"SPAN", "CENTER", SpanCenter},
                                {"AVERAGE", "PEAK OFF", AveragePeak}, {"FIXED2", "FREEZE", FixedFreeze},
                                {"CURS A+", "CURS B+", CursAB}};

    for (const auto &def : items) {
        auto *btn = new DisplayMenuButton(def.primary, def.alternate, this);
        m_menuButtons.append(btn);

        connect(btn, &DisplayMenuButton::clicked, this, [this, item = def.item]() { onMenuItemClicked(item); });
        connect(btn, &DisplayMenuButton::rightClicked, this,
                [this, item = def.item]() { onMenuItemRightClicked(item); });

        bottomRow->addWidget(btn);
    }

    static_cast<QVBoxLayout *>(layout())->addLayout(bottomRow);
}

void DisplayPopupWidget::onMenuItemClicked(MenuItem item) {
    m_selectedItem = item;
    updateMenuButtonStyles();

    // Switch control page for items that show controls
    switch (item) {
    case SpanCenter:
        m_controlStack->setCurrentWidget(m_spanControlPage);
        break;
    case RefLvlScale:
        m_controlStack->setCurrentWidget(m_refLevelControlPage);
        break;
    case AveragePeak:
        m_controlStack->setCurrentWidget(m_averageControlPage);
        break;
    case NbWtrClrs:
        m_controlStack->setCurrentWidget(m_nbControlPage);
        break;
    default:
        m_controlStack->setCurrentWidget(m_defaultControlPage);
        break;
    }

    // Emit CAT commands based on menu item
    switch (item) {
    case PanWaterfall: {
        // Cycle DPM: 0 → 1 → 2 → 0 (use current mode based on LCD/EXT selection)
        int currentMode = (m_extEnabled && !m_lcdEnabled) ? m_dualPanModeExt : m_dualPanModeLcd;
        if (currentMode < 0)
            currentMode = 0; // Handle uninitialized state
        int newMode = (currentMode + 1) % 3;

        // Send CAT command to radio
        if (m_lcdEnabled) {
            emit catCommandRequested(QString("#DPM%1;").arg(newMode));
        }
        if (m_extEnabled) {
            emit catCommandRequested(QString("#HDPM%1;").arg(newMode));
        }

        // Update local state immediately (K4 doesn't echo #DPM commands)
        if (m_lcdEnabled) {
            m_dualPanModeLcd = newMode;
        }
        if (m_extEnabled) {
            m_dualPanModeExt = newMode;
        }
        updateMenuButtonLabels();

        // Auto-sync A/B toggle with PAN mode (so Center/Span target correct VFO)
        switch (newMode) {
        case 0: // A only
            m_vfoAEnabled = true;
            m_vfoBEnabled = false;
            break;
        case 1: // B only
            m_vfoAEnabled = false;
            m_vfoBEnabled = true;
            break;
        case 2: // Dual (A+B)
            m_vfoAEnabled = true;
            m_vfoBEnabled = true;
            break;
        }
        updateToggleStyles();
        updateRefLevelControlGroup(); // Sync ref level display with A/B
        updateSpanControlGroup();     // Sync span display with A/B

        // Notify MainWindow to update panadapter display
        emit dualPanModeChanged(newMode);
        break;
    }
    case NbWtrClrs:
        // Toggle NB - handled elsewhere via existing NB command
        emit catCommandRequested("NB;");
        break;
    case AveragePeak: {
        // Cycle averaging: 1 → 5 → 10 → 15 → 20 → 1
        static const int avgSteps[] = {1, 5, 10, 15, 20};
        int idx = 0;
        for (int i = 0; i < 5; ++i) {
            if (m_averaging == avgSteps[i]) {
                idx = (i + 1) % 5;
                break;
            }
        }
        int newAvg = avgSteps[idx];
        if (m_lcdEnabled) {
            emit catCommandRequested(QString("#AVG%1;").arg(newAvg, 2, 10, QChar('0')));
        }
        if (m_extEnabled) {
            emit catCommandRequested(QString("#HAVG%1;").arg(newAvg, 2, 10, QChar('0')));
        }
        break;
    }
    case FixedFreeze: {
        // Cycle through 6 fixed tune modes: SLIDE1 → SLIDE2 → FIXED1 → FIXED2 → STATIC → TRACK → SLIDE1
        // Internal state: 0=TRACK, 1=SLIDE1, 2=SLIDE2, 3=FIXED1, 4=FIXED2, 5=STATIC
        // Cycle order: 1 → 2 → 3 → 4 → 5 → 0 → 1
        int newMode = (m_fixedTuneMode % 6) + 1;
        if (newMode > 5)
            newMode = 0;
        if (newMode == 6)
            newMode = 0;

        // Map internal state to FXT/FXA commands
        if (newMode == 0) {
            // TRACK: FXT=0
            emit catCommandRequested("#FXT0;");
        } else {
            // Fixed modes: FXT=1, then set FXA
            emit catCommandRequested("#FXT1;");
            int fxa;
            switch (newMode) {
            case 1:
                fxa = 0;
                break; // SLIDE1: FXA=0
            case 2:
                fxa = 4;
                break; // SLIDE2: FXA=4
            case 3:
                fxa = 1;
                break; // FIXED1: FXA=1
            case 4:
                fxa = 2;
                break; // FIXED2: FXA=2
            case 5:
                fxa = 3;
                break; // STATIC: FXA=3
            default:
                fxa = 0;
                break;
            }
            emit catCommandRequested(QString("#FXA%1;").arg(fxa));
        }
        break;
    }
    case CursAB:
        // Cycle VFO A cursor mode
        emit catCommandRequested("#VFA/;");
        break;
    default:
        break;
    }

    emit menuItemSelected(item);
}

void DisplayPopupWidget::onMenuItemRightClicked(MenuItem item) {
    // Handle right-click (alternate action) for each menu item
    switch (item) {
    case PanWaterfall: {
        // Cycle display mode: 0 (spectrum) ↔ 1 (spectrum+waterfall)
        int currentMode = (m_extEnabled && !m_lcdEnabled) ? m_displayModeExt : m_displayModeLcd;
        int newMode = currentMode == 0 ? 1 : 0;
        if (m_lcdEnabled) {
            emit catCommandRequested(QString("#DSM%1;").arg(newMode));
        }
        if (m_extEnabled) {
            emit catCommandRequested(QString("#HDSM%1;").arg(newMode));
        }
        break;
    }
    case NbWtrClrs: {
        // Cycle waterfall color: 0 → 1 → 2 → 3 → 4 → 0
        int newColor = (m_waterfallColor + 1) % 5;
        // Use $ suffix for VFO B if selected
        QString suffix = m_vfoBEnabled && !m_vfoAEnabled ? "$" : "";
        if (m_lcdEnabled) {
            emit catCommandRequested(QString("#WFC%1%2;").arg(suffix).arg(newColor));
        }
        if (m_extEnabled) {
            emit catCommandRequested(QString("#HWFC%1%2;").arg(suffix).arg(newColor));
        }
        break;
    }
    case RefLvlScale:
        // Toggle auto-ref
        emit catCommandRequested("#AR/;");
        break;
    case SpanCenter: {
        // Center on VFO
        QString suffix = m_vfoBEnabled && !m_vfoAEnabled ? "$" : "";
        emit catCommandRequested(QString("FC%1;").arg(suffix));
        break;
    }
    case AveragePeak:
        // Toggle peak mode
        emit catCommandRequested("#PKM/;");
        break;
    case FixedFreeze: {
        // Toggle freeze
        int newFreeze = m_freeze > 0 ? 0 : 1;
        emit catCommandRequested(QString("#FRZ%1;").arg(newFreeze));
        break;
    }
    case CursAB:
        // Cycle VFO B cursor mode
        emit catCommandRequested("#VFB/;");
        break;
    default:
        break;
    }

    emit alternateItemClicked(item);
}

void DisplayPopupWidget::updateToggleStyles() {
    // Update toggle group widgets
    if (m_lcdExtGroup) {
        m_lcdExtGroup->setLeftSelected(m_lcdEnabled);
        m_lcdExtGroup->setRightSelected(m_extEnabled);
    }
    if (m_vfoAbGroup) {
        m_vfoAbGroup->setLeftSelected(m_vfoAEnabled);
        m_vfoAbGroup->setRightSelected(m_vfoBEnabled);
        m_vfoAbGroup->setRightEnabled(m_vfoBAvailable);
    }
}

void DisplayPopupWidget::updateMenuButtonStyles() {
    for (int i = 0; i < m_menuButtons.size(); ++i) {
        m_menuButtons[i]->setSelected(i == static_cast<int>(m_selectedItem));
    }
}

void DisplayPopupWidget::showAboveButton(QWidget *triggerButton) {
    if (!triggerButton)
        return;

    // Get the trigger button's global position
    QPoint btnGlobal = triggerButton->mapToGlobal(QPoint(0, 0));
    int btnCenterX = btnGlobal.x() + triggerButton->width() / 2;

    // Calculate popup position (centered above button, with triangle pointing down)
    int popupX = btnCenterX - width() / 2;
    int popupY = btnGlobal.y() - height();

    // Store offset for triangle drawing (relative to popup center)
    m_triangleXOffset = 0;

    // Ensure popup stays on screen
    QRect screenGeom = QApplication::primaryScreen()->availableGeometry();
    if (popupX < screenGeom.left()) {
        m_triangleXOffset = popupX - screenGeom.left();
        popupX = screenGeom.left();
    } else if (popupX + width() > screenGeom.right()) {
        m_triangleXOffset = (popupX + width()) - screenGeom.right();
        popupX = screenGeom.right() - width();
    }

    move(popupX, popupY);
    show();
    setFocus();
    update();
}

void DisplayPopupWidget::hidePopup() {
    hide();
    // closed() signal is emitted in hideEvent()
}

void DisplayPopupWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Main popup area (everything except the triangle that extends below)
    int mainHeight = height() - TriangleHeight;
    QRect mainRect(0, 0, width(), mainHeight);

    // Fill main background
    painter.fillRect(mainRect, QColor(30, 30, 30));

    // Gray bottom strip (inside the main rect, at bottom)
    QRect stripRect(0, mainHeight - BottomStripHeight, width(), BottomStripHeight);
    painter.fillRect(stripRect, IndicatorColor);

    // Border around main popup
    painter.setPen(QPen(QColor(60, 60, 60), 1));
    painter.drawRect(mainRect.adjusted(0, 0, -1, -1));

    // Triangle pointing down from center of bottom strip
    painter.setPen(Qt::NoPen);
    painter.setBrush(IndicatorColor);
    int triangleX = width() / 2 + m_triangleXOffset;
    QPainterPath triangle;
    triangle.moveTo(triangleX - TriangleWidth / 2, mainHeight);
    triangle.lineTo(triangleX + TriangleWidth / 2, mainHeight);
    triangle.lineTo(triangleX, height());
    triangle.closeSubpath();
    painter.drawPath(triangle);
}

void DisplayPopupWidget::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        hidePopup();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void DisplayPopupWidget::focusOutEvent(QFocusEvent *event) {
    Q_UNUSED(event)
    // Qt::Popup handles auto-close on focus loss
}

void DisplayPopupWidget::setSpanValueA(double spanKHz) {
    m_spanA = spanKHz;
    updateSpanControlGroup();
}

void DisplayPopupWidget::setSpanValueB(double spanKHz) {
    m_spanB = spanKHz;
    updateSpanControlGroup();
}

void DisplayPopupWidget::updateSpanControlGroup() {
    if (!m_spanControlGroup)
        return;
    // Show correct value based on A/B toggle
    bool useB = m_vfoBEnabled && !m_vfoAEnabled;
    double value = useB ? m_spanB : m_spanA;
    m_spanControlGroup->setValue(QString::number(value, 'f', 1));
}

void DisplayPopupWidget::setRefLevelValue(int dB) {
    // Legacy: forward to A
    setRefLevelValueA(dB);
}

void DisplayPopupWidget::setAutoRefLevel(bool enabled) {
    // Auto-ref is GLOBAL - affects both VFOs
    m_autoRef = enabled;
    updateRefLevelControlGroup();
}

void DisplayPopupWidget::setRefLevelValueA(int dB) {
    m_refLevelA = dB;
    m_currentRefLevel = dB; // Keep legacy in sync
    updateRefLevelControlGroup();
}

void DisplayPopupWidget::setRefLevelValueB(int dB) {
    m_refLevelB = dB;
    updateRefLevelControlGroup();
}

void DisplayPopupWidget::updateRefLevelControlGroup() {
    if (!m_refLevelControlGroup)
        return;

    // Determine which ref level VALUE to show based on A/B toggle
    // (Values are per-VFO, but auto mode is GLOBAL)
    bool useB = m_vfoBEnabled && !m_vfoAEnabled;
    int value = useB ? m_refLevelB : m_refLevelA;

    m_refLevelControlGroup->setValue(QString::number(value));
    m_refLevelControlGroup->setAutoEnabled(m_autoRef); // GLOBAL auto mode
    m_refLevelControlGroup->setValueFaded(m_autoRef);  // Faded when auto
}

void DisplayPopupWidget::hideEvent(QHideEvent *event) {
    Q_UNUSED(event)
    // Emit closed signal whenever popup is hidden (whether by hidePopup() or Qt::Popup auto-close)
    emit closed();
}

// ============================================================================
// State Setters (called by RadioState signals)
// ============================================================================

void DisplayPopupWidget::setDualPanModeLcd(int mode) {
    if (m_dualPanModeLcd != mode) {
        m_dualPanModeLcd = mode;
        // Update labels if LCD is selected
        if (m_lcdEnabled) {
            updateMenuButtonLabels();

            // Auto-sync A/B toggle with PAN mode (for initial connect sync)
            switch (mode) {
            case 0: // A only
                m_vfoAEnabled = true;
                m_vfoBEnabled = false;
                break;
            case 1: // B only
                m_vfoAEnabled = false;
                m_vfoBEnabled = true;
                break;
            case 2: // Dual (A+B)
                m_vfoAEnabled = true;
                m_vfoBEnabled = true;
                break;
            }
            updateToggleStyles();
            updateRefLevelControlGroup(); // Sync ref level display with A/B
            updateSpanControlGroup();     // Sync span display with A/B
        }
    }
}

void DisplayPopupWidget::setDualPanModeExt(int mode) {
    if (m_dualPanModeExt != mode) {
        m_dualPanModeExt = mode;
        // Update labels if EXT only is selected
        if (m_extEnabled && !m_lcdEnabled) {
            updateMenuButtonLabels();
        }
    }
}

void DisplayPopupWidget::setDisplayModeLcd(int mode) {
    if (m_displayModeLcd != mode) {
        m_displayModeLcd = mode;
        if (m_lcdEnabled) {
            updateMenuButtonLabels();
        }
    }
}

void DisplayPopupWidget::setDisplayModeExt(int mode) {
    if (m_displayModeExt != mode) {
        m_displayModeExt = mode;
        if (m_extEnabled && !m_lcdEnabled) {
            updateMenuButtonLabels();
        }
    }
}

void DisplayPopupWidget::setWaterfallColor(int color) {
    if (m_waterfallColor != color) {
        m_waterfallColor = color;
        updateMenuButtonLabels();
    }
}

void DisplayPopupWidget::setAveraging(int value) {
    if (m_averaging != value) {
        m_averaging = value;
        if (m_averageControlGroup) {
            m_averageControlGroup->setValue(QString::number(value));
        }
        updateMenuButtonLabels();
    }
}

void DisplayPopupWidget::setPeakMode(bool enabled) {
    if (m_peakMode != enabled) {
        m_peakMode = enabled;
        updateMenuButtonLabels();
    }
}

void DisplayPopupWidget::setFixedTuneMode(int fxt, int fxa) {
    // Combine FXT and FXA into internal state (0-5)
    // TRACK=0: FXT=0 (doesn't matter what FXA is)
    // SLIDE1=1: FXT=1, FXA=0 (FULL SPAN)
    // SLIDE2=2: FXT=1, FXA=4 (SLIDE NEAR EDGE)
    // FIXED1=3: FXT=1, FXA=1 (HALF SPAN)
    // FIXED2=4: FXT=1, FXA=2 (SLIDE EDGE)
    // STATIC=5: FXT=1, FXA=3 (STATIC)
    int newMode;
    if (fxt == 0) {
        newMode = 0; // TRACK
    } else {
        // FXT=1, map FXA to internal state
        switch (fxa) {
        case 0:
            newMode = 1;
            break; // SLIDE1
        case 4:
            newMode = 2;
            break; // SLIDE2
        case 1:
            newMode = 3;
            break; // FIXED1
        case 2:
            newMode = 4;
            break; // FIXED2
        case 3:
            newMode = 5;
            break; // STATIC
        default:
            newMode = 0;
            break;
        }
    }

    if (m_fixedTuneMode != newMode) {
        m_fixedTuneMode = newMode;
        updateMenuButtonLabels();
    }
}

void DisplayPopupWidget::setFreeze(bool enabled) {
    if (m_freeze != enabled) {
        m_freeze = enabled;
        updateMenuButtonLabels();
    }
}

void DisplayPopupWidget::setVfoACursor(int mode) {
    if (m_vfaMode != mode) {
        m_vfaMode = mode;
        updateMenuButtonLabels();
    }
}

void DisplayPopupWidget::setVfoBCursor(int mode) {
    if (m_vfbMode != mode) {
        m_vfbMode = mode;
        updateMenuButtonLabels();
    }
}

void DisplayPopupWidget::setDdcNbMode(int mode) {
    if (m_ddcNbMode != mode) {
        m_ddcNbMode = mode;
        updateNbControlGroupValue();
    }
}

void DisplayPopupWidget::setDdcNbLevel(int level) {
    if (m_ddcNbLevel != level) {
        m_ddcNbLevel = level;
        updateNbControlGroupValue();
    }
}

// ============================================================================
// Button Label Updates
// ============================================================================

void DisplayPopupWidget::updateMenuButtonLabels() {
    if (m_menuButtons.size() < 7)
        return;

    // Use LCD or EXT state based on selection
    int panMode = (m_extEnabled && !m_lcdEnabled) ? m_dualPanModeExt : m_dualPanModeLcd;
    int displayMode = (m_extEnabled && !m_lcdEnabled) ? m_displayModeExt : m_displayModeLcd;

    // PanWaterfall button (index 0)
    QString panText;
    switch (panMode) {
    case 0:
        panText = "PAN = A";
        break;
    case 1:
        panText = "PAN = B";
        break;
    case 2:
        panText = "PAN = A+B";
        break;
    default:
        panText = "PAN = A";
        break;
    }
    m_menuButtons[0]->setPrimaryText(panText);
    m_menuButtons[0]->setAlternateText(displayMode == 0 ? "SPECTRUM" : "WTRFALL");

    // NbWtrClrs button (index 1) - alternate text shows waterfall color
    static const char *colorNames[] = {"WTR GRAY", "WTR COLOR", "WTR TEAL", "WTR BLUE", "WTR SEPIA"};
    if (m_waterfallColor >= 0 && m_waterfallColor <= 4) {
        m_menuButtons[1]->setAlternateText(colorNames[m_waterfallColor]);
    }

    // RefLvlScale button (index 2) - no dynamic text for now

    // SpanCenter button (index 3) - no dynamic text for now

    // AveragePeak button (index 4) - primary text stays as "AVERAGE"
    m_menuButtons[4]->setAlternateText(m_peakMode > 0 ? "PEAK ON" : "PEAK OFF");

    // FixedFreeze button (index 5)
    static const char *fixedModeNames[] = {"TRACK", "SLIDE1", "SLIDE2", "FIXED1", "FIXED2", "STATIC"};
    if (m_fixedTuneMode >= 0 && m_fixedTuneMode <= 5) {
        m_menuButtons[5]->setPrimaryText(fixedModeNames[m_fixedTuneMode]);
    }
    m_menuButtons[5]->setAlternateText(m_freeze > 0 ? "FREEZE" : "RUN");

    // CursAB button (index 6)
    // OFF=hide, ON=show, AUTO=show, HIDE=hide
    static const char *cursorNames[] = {"CURS A-", "CURS A+", "CURS A+", "CURS A-"};
    static const char *cursorBNames[] = {"CURS B-", "CURS B+", "CURS B+", "CURS B-"};
    if (m_vfaMode >= 0 && m_vfaMode <= 3) {
        m_menuButtons[6]->setPrimaryText(cursorNames[m_vfaMode]);
    }
    if (m_vfbMode >= 0 && m_vfbMode <= 3) {
        m_menuButtons[6]->setAlternateText(cursorBNames[m_vfbMode]);
    }
}

QString DisplayPopupWidget::getCommandPrefix() const {
    // Returns "H" for EXT-only mode, empty string otherwise
    if (m_extEnabled && !m_lcdEnabled) {
        return "H";
    }
    return "";
}
