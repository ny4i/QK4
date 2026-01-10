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
| Namespaces | PascalCase | `K4Styles`, `K4Styles::Colors` |

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

## Color Palette (K4Styles::Colors)

**Source of truth:** `src/ui/k4styles.h` - All colors are `constexpr const char*` in `K4Styles::Colors::*`

```cpp
// App Accent Color
K4Styles::Colors::AccentAmber    // "#FFB000" - TX indicators, labels, highlights

// VFO Theme Colors (square, passband, markers, overlays)
K4Styles::Colors::VfoACyan       // "#00BFFF" - VFO A: cyan theme
K4Styles::Colors::VfoBGreen      // "#00FF00" - VFO B: green theme

// Backgrounds
K4Styles::Colors::Background        // "#1a1a1a"
K4Styles::Colors::DarkBackground    // "#0d0d0d"
K4Styles::Colors::PopupBackground   // "#1e1e1e"

// Text
K4Styles::Colors::TextWhite      // "#FFFFFF"
K4Styles::Colors::TextDark       // "#333333"
K4Styles::Colors::TextGray       // "#999999"
K4Styles::Colors::TextFaded      // "#808080"
K4Styles::Colors::InactiveGray   // "#666666"

// Status
K4Styles::Colors::TxRed          // "#FF0000"
K4Styles::Colors::AgcGreen       // "#00FF00"
K4Styles::Colors::RitCyan        // "#00CED1"
```

### Spectrum Colors

- **Main Panadapter**: QRhi gradient (green fill with lime line)
- **Mini-Pan Line**: VfoACyan `#00BFFF` for A, VfoBGreen `#00FF00` for B
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

## Popup & Button Styling (K4Styles)

**Source of truth:** `src/ui/k4styles.h` - Use K4Styles functions instead of inline CSS.

### Stylesheet Functions (for QPushButton)

| Function | Usage |
|----------|-------|
| `K4Styles::popupButtonNormal()` | Standard dark gradient popup buttons |
| `K4Styles::popupButtonSelected()` | Light/white selected state |
| `K4Styles::menuBarButton()` | Bottom menu bar buttons (with padding) |
| `K4Styles::menuBarButtonActive()` | Active/inverted menu button state |
| `K4Styles::menuBarButtonSmall()` | Compact +/- buttons |

### QPainter Helpers (for custom-painted widgets)

```cpp
// Gradients
QLinearGradient K4Styles::buttonGradient(top, bottom, hovered)
QLinearGradient K4Styles::selectedGradient(top, bottom)

// Colors
QColor K4Styles::borderColor()         // Normal border
QColor K4Styles::borderColorSelected() // Selected border

// Shadow
K4Styles::drawDropShadow(painter, contentRect, cornerRadius)
```

### Dimension Constants (K4Styles::Dimensions::*)

| Constant | Value | Purpose |
|----------|-------|---------|
| `ShadowMargin` | 20 | Space around popup for shadow |
| `PopupContentMargin` | 12 | Padding inside popup |
| `PopupButtonWidth` | 70 | Standard popup button |
| `PopupButtonHeight` | 44 | Standard popup button |
| `BorderWidth` | 2 | Button border width |
| `BorderRadius` | 6 | Standard corner radius |

**Creating popups:** See `PATTERNS.md` → "Adding a Popup Menu" and `src/ui/CLAUDE.md` → K4PopupBase section.
