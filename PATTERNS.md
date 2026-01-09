# Patterns

Signal/slot patterns and feature addition guides for K4Controller.

## Signal/Slot Connection Patterns

### RadioState → MainWindow

```cpp
connect(m_radioState, &RadioState::frequencyChanged,
        this, &MainWindow::onFrequencyChanged);
connect(m_radioState, &RadioState::modeChanged,
        this, &MainWindow::onModeChanged);
```

### Widget → Lambda (inline handling)

```cpp
connect(m_spanUpBtn, &QPushButton::clicked, this, [this]() {
    int newSpan = m_panadapterA->span() + 1000;
    m_tcpClient->sendCAT(QString("SPAN%1;").arg(newSpan));
});
```

### Custom Widget Signals

```cpp
// In PanadapterWidget
signals:
    void frequencyClicked(qint64 frequency);
    void frequencyScrolled(int direction);

// Connection in MainWindow
connect(m_panadapterA, &PanadapterWidget::frequencyClicked,
        this, [this](qint64 freq) {
    m_tcpClient->sendCAT(QString("FA%1;").arg(freq, 11, 10, QChar('0')));
});
```

---

## Adding New Features

### Adding a New Widget

1. Create `src/<category>/<widgetname>.cpp/.h`
2. Subclass `QWidget`, implement `paintEvent()` if custom drawing
3. Add to `CMakeLists.txt` SOURCES and HEADERS
4. Include in `mainwindow.h`, create instance in `setupUi()`
5. Connect signals/slots in MainWindow constructor

**GPU-accelerated widgets:** For high-performance rendering (spectrum, waterfall), subclass `QRhiWidget` instead of `QWidget`. See `src/dsp/panadapter_rhi.cpp` for the pattern. Requires shader compilation via `qt6_add_shaders()` in CMakeLists.txt.

### Adding a New Menu Item

```cpp
// In MainWindow::setupMenuBar()
QMenu *myMenu = menuBar()->addMenu("&MyMenu");
QAction *myAction = myMenu->addAction("My Action");
connect(myAction, &QAction::triggered, this, &MainWindow::onMyAction);
```

### Adding RadioState Property

1. Add member variable and getter in `radiostate.h`
2. Add signal: `void myPropertyChanged(Type value);`
3. Add setter that emits signal on change
4. Parse in `Protocol::processCAT()` and call setter
5. Connect signal to UI update slot in MainWindow

### Adding a CAT Command

1. **models/radiostate.cpp**: Add parser case in `parseCATCommand()`
2. **models/radiostate.h**: Add getter, signal, member variable
3. **mainwindow.cpp**: Connect `RadioState::*Changed()` to UI update slot

### Adding Protocol Packet Type

1. **network/protocol.h**: Add to `PayloadType` enum
2. **network/protocol.cpp**: Add case in `processPacket()`
3. **network/protocol.h**: Add signal for new packet type
4. **mainwindow.cpp**: Connect signal to handler

---

## Adding a Popup Menu

**Template files to reference:**
- `src/ui/bandpopupwidget.cpp` - Best template for grid popups with selection
- `src/ui/buttonrowpopup.cpp` - Simple single-row popup
- `src/ui/k4styles.h` - Required for button styles

### Step 1: Create Header File

```cpp
// src/ui/mypopup.h
#ifndef MYPOPUP_H
#define MYPOPUP_H

#include <QWidget>
#include <QPushButton>
#include <QList>

class MyPopup : public QWidget {
    Q_OBJECT

public:
    explicit MyPopup(QWidget *parent = nullptr);
    void showAboveButton(QWidget *triggerButton);
    void hidePopup();

signals:
    void itemSelected(int index);  // Or QString, etc.
    void closed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void setupUi();
    QList<QPushButton *> m_buttons;
    int m_triangleXOffset = 0;
};

#endif // MYPOPUP_H
```

### Step 2: Create Implementation File

```cpp
// src/ui/mypopup.cpp
#include "mypopup.h"
#include "k4styles.h"  // REQUIRED for button styles
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QHideEvent>
#include <QApplication>
#include <QScreen>

namespace {
// Copy these constants from bandpopupwidget.cpp or buttonrowpopup.cpp
const QColor IndicatorColor(85, 85, 85);
const int ButtonWidth = 70;
const int ButtonHeight = 44;
const int ButtonSpacing = 8;
const int ContentMargin = 12;
const int TriangleWidth = 24;
const int TriangleHeight = 12;
const int BottomStripHeight = 8;
const int ShadowRadius = 16;
const int ShadowOffsetX = 2;
const int ShadowOffsetY = 4;
const int ShadowMargin = ShadowRadius + 4;
}

MyPopup::MyPopup(QWidget *parent) : QWidget(parent), m_triangleXOffset(0) {
    // REQUIRED window flags for popup behavior
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);  // Required for shadow
    setFocusPolicy(Qt::StrongFocus);
    setupUi();
}

void MyPopup::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    // Margins include shadow space + content padding + bottom strip + triangle
    mainLayout->setContentsMargins(
        ShadowMargin + ContentMargin,
        ShadowMargin + ContentMargin,
        ShadowMargin + ContentMargin,
        ShadowMargin + ContentMargin + BottomStripHeight + TriangleHeight
    );
    mainLayout->setSpacing(0);

    auto *rowLayout = new QHBoxLayout();
    rowLayout->setSpacing(ButtonSpacing);

    // Create buttons - USE K4Styles!
    for (int i = 0; i < 7; ++i) {
        auto *btn = new QPushButton(QString::number(i + 1), this);
        btn->setFixedSize(ButtonWidth, ButtonHeight);
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setProperty("buttonIndex", i);
        btn->setStyleSheet(K4Styles::popupButtonNormal());  // REQUIRED
        connect(btn, &QPushButton::clicked, this, [this, i]() {
            emit itemSelected(i);
            hidePopup();
        });
        m_buttons.append(btn);
        rowLayout->addWidget(btn);
    }
    mainLayout->addLayout(rowLayout);

    // Calculate and set fixed size
    int contentWidth = 7 * ButtonWidth + 6 * ButtonSpacing + 2 * ContentMargin;
    int contentHeight = ButtonHeight + 2 * ContentMargin + BottomStripHeight + TriangleHeight;
    setFixedSize(contentWidth + 2 * ShadowMargin, contentHeight + 2 * ShadowMargin);
}

// Copy paintEvent from bandpopupwidget.cpp or buttonrowpopup.cpp
// Copy showAboveButton from bandpopupwidget.cpp (handles positioning + triangle)
// Copy hidePopup, hideEvent, keyPressEvent from any popup
```

### Step 3: Add to CMakeLists.txt

```cmake
# In SOURCES section:
src/ui/mypopup.cpp

# In HEADERS section:
src/ui/mypopup.h
```

### Step 4: Wire Up in MainWindow

```cpp
// mainwindow.h - Add member
MyPopup *m_myPopup;

// mainwindow.cpp - Create and connect
m_myPopup = new MyPopup(this);
connect(m_myPopup, &MyPopup::itemSelected, this, &MainWindow::onMyItemSelected);
connect(m_myPopup, &MyPopup::closed, this, [this]() {
    // Reset trigger button state if needed
});

// Show popup when trigger button clicked
connect(m_triggerButton, &QPushButton::clicked, this, [this]() {
    m_myPopup->showAboveButton(m_triggerButton);
});
```

### Key Implementation Details

**Shadow Drawing (copy from bandpopupwidget.cpp paintEvent):**
```cpp
// Draw 8-layer soft shadow
painter.setPen(Qt::NoPen);
const int shadowLayers = 8;
for (int i = shadowLayers; i > 0; --i) {
    int blur = i * 2;
    int alpha = 12 + (shadowLayers - i) * 3;
    QRect shadowRect = contentRect.adjusted(-blur, -blur, blur, blur);
    shadowRect.translate(ShadowOffsetX, ShadowOffsetY);
    painter.setBrush(QColor(0, 0, 0, alpha));
    painter.drawRoundedRect(shadowRect, 8 + blur / 2, 8 + blur / 2);
}
```

**Triangle Pointer (copy from bandpopupwidget.cpp):**
```cpp
// Triangle pointing to trigger button
int triangleX = contentRect.center().x() + m_triangleXOffset;
int triangleTop = contentRect.bottom() + 1;
QPainterPath triangle;
triangle.moveTo(triangleX - TriangleWidth / 2, triangleTop);
triangle.lineTo(triangleX + TriangleWidth / 2, triangleTop);
triangle.lineTo(triangleX, triangleTop + TriangleHeight);
triangle.closeSubpath();
painter.drawPath(triangle);
```

**For popups with selected state:**
```cpp
void updateButtonStyles() {
    for (auto *btn : m_buttons) {
        if (btn == m_selectedButton) {
            btn->setStyleSheet(K4Styles::popupButtonSelected());
        } else {
            btn->setStyleSheet(K4Styles::popupButtonNormal());
        }
    }
}
```
