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
