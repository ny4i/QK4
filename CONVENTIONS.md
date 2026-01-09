# Conventions

Code style and naming conventions for K4Controller.

## Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Classes | PascalCase | `PanadapterWidget`, `RadioState` |
| Methods | camelCase | `updateSpectrum()`, `setRefLevel()` |
| Member Variables | m_camelCase | `m_frequencyALabel`, `m_tcpClient` |
| Signals | camelCase | `frequencyChanged`, `clicked` |
| Slots | on + Source + Event | `onFrequencyChanged`, `onConnectClicked` |
| Constants | SCREAMING_SNAKE | `WATERFALL_HISTORY`, `TOTAL_BANDWIDTH_HZ` |
| Namespaces | PascalCase | `K4Colors`, `MiniPanColors` |

## Member Variable Prefixes

```cpp
// Pointers to UI elements
QLabel *m_frequencyALabel;      // Labels: m_<purpose>Label
QPushButton *m_spanUpBtn;       // Buttons: m_<name>Btn
SMeterWidget *m_sMeterA;        // Widgets: m_<name> or m_<name><VFO>
QStackedWidget *m_vfoAStackedWidget;

// A/B suffix for dual-VFO components
m_frequencyALabel, m_frequencyBLabel
m_sMeterA, m_sMeterB
m_panadapterA, m_panadapterB
```

## Qt Patterns

- **Signal/Slot**: Use `connect()` with lambda or member function pointers
- **Binary Data**: Use `QByteArray` for protocol data
- **Layouts**: Prefer `QVBoxLayout`/`QHBoxLayout`, use `QStackedWidget` for page switching
- **Styling**: Use `setStyleSheet()` with CSS-like syntax
- **Custom Painting**: Override `paintEvent()`, use `QPainter` with `QPainterPath`

## Code Formatting (clang-format)

| Aspect | Style |
|--------|-------|
| Indentation | 4 spaces |
| Line length | 120 characters max |
| Braces | Same line (Attach) |
| Pointers | Right-aligned (`Type *name`) |
| Includes | Preserve order (no auto-sort) |

```bash
# Check formatting
find src -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run --Werror

# Auto-format
find src -name '*.cpp' -o -name '*.h' | xargs clang-format -i
```

### Example

```cpp
void MainWindow::onFrequencyChanged(quint64 freq)
{
    if (freq > 0) {
        m_frequencyLabel->setText(QString::number(freq));
        m_radioState->setFrequency(freq);
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_tcpClient(new TcpClient(this)),
      m_radioState(new RadioState(this))
{
    setupUi();
}
```

## Color Palette (K4Colors Namespace)

```cpp
namespace K4Colors {
    const char* Background      = "#1a1a1a";  // Main background
    const char* DarkBackground  = "#0d0d0d";  // Status bars, menus
    const char* VfoAAmber       = "#FFB000";  // VFO A, TX indicator
    const char* VfoBCyan        = "#00BFFF";  // VFO B
    const char* TxRed           = "#FF0000";  // Error states
    const char* AgcGreen        = "#00FF00";  // Active/connected
    const char* InactiveGray    = "#666666";  // Disabled controls
    const char* TextWhite       = "#FFFFFF";  // Primary text
    const char* TextGray        = "#999999";  // Secondary text
    const char* RitCyan         = "#00CED1";  // RIT/XIT display
}
```

### Spectrum Colors

- **Main Panadapter Line**: Lime green `#32FF32` with gradient fill
- **Mini-Pan Line**: Amber `#FFB000`
- **Waterfall**: 8-stage LUT (Black → Blue → Cyan → Green → Yellow → Red)

## Fonts

Embedded HD fonts for crisp rendering on Retina/4K displays.

| Font | Type | Usage |
|------|------|-------|
| **Inter** | Sans-serif | Default UI font (Medium weight, 11pt) |
| **JetBrains Mono** | Monospace | Frequency display, CAT commands |

### Font Sizes

| Size | Usage |
|------|-------|
| 32px | VFO frequency display |
| 18px | TX indicator |
| 14px | Title, RIT/XIT value |
| 12px | Status bar labels |
| 11px | Mode labels, indicators |
| 9-10px | Small indicators, mini-pan |

### Stylesheet Usage

```cpp
// UI text (uses Inter automatically via QApplication::setFont)
label->setStyleSheet("font-size: 12px; font-weight: bold;");

// Frequency/data display (explicit JetBrains Mono)
label->setStyleSheet("font-family: 'JetBrains Mono', monospace; font-size: 32px;");
```

## Popup & Button Styling

**Source of truth:** `src/ui/k4styles.h` - Use K4Styles functions instead of inline CSS.

| Function | Usage |
|----------|-------|
| `K4Styles::popupButtonNormal()` | Standard dark gradient popup buttons |
| `K4Styles::popupButtonSelected()` | Light/white selected state |
| `K4Styles::menuBarButton()` | Bottom menu bar buttons (with padding) |
| `K4Styles::menuBarButtonActive()` | Active/inverted menu button state |
| `K4Styles::menuBarButtonSmall()` | Compact +/- buttons |

**Constants:** Use `K4Styles::Dimensions::*` for shadow/border values, `K4Styles::Colors::*` for colors.

**Creating popups:** See `PATTERNS.md` → "Adding a Popup Menu" and `src/ui/CLAUDE.md` → "Creating New Popups".
