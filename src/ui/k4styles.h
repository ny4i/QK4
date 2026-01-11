#ifndef K4STYLES_H
#define K4STYLES_H

#include <QLinearGradient>
#include <QPainter>
#include <QString>

/**
 * @brief Centralized styling for K4Controller UI components.
 *
 * This namespace provides consistent button and popup styles across all widgets,
 * eliminating duplicate CSS definitions and ensuring visual consistency.
 *
 * Style Categories:
 * - Popup buttons: Used in BandPopup, ModePopup, ButtonRowPopup, DisplayPopup
 * - Menu bar buttons: Used in BottomMenuBar, FeatureMenuBar
 * - Selected/Active states: Inverted colors for selected items
 */
namespace K4Styles {

// =============================================================================
// Popup Button Styles (BandPopup, ModePopup, ButtonRowPopup, DisplayPopup)
// =============================================================================

/**
 * @brief Standard dark gradient button for popup widgets.
 * Used for unselected band/mode buttons, popup action buttons.
 */
QString popupButtonNormal();

/**
 * @brief Light/white button for selected state in popups.
 * Used for currently selected band/mode.
 */
QString popupButtonSelected();

// =============================================================================
// Menu Bar Button Styles (BottomMenuBar, FeatureMenuBar)
// =============================================================================

/**
 * @brief Standard button for bottom menu bar (MENU, Fn, DISPLAY, BAND, etc.)
 * Includes padding suitable for text labels.
 */
QString menuBarButton();

/**
 * @brief Active/pressed state for menu bar buttons.
 * White background with dark text (inverted from normal).
 */
QString menuBarButtonActive();

/**
 * @brief Compact button for +/- controls in FeatureMenuBar.
 * Same gradient but no padding, larger font.
 */
QString menuBarButtonSmall();

// =============================================================================
// Common Style Constants
// =============================================================================

namespace Colors {
// =============================================================================
// App-Level Theme Colors (consolidated from K4Colors namespaces)
// =============================================================================

// Backgrounds
constexpr const char *Background = "#1a1a1a";
constexpr const char *DarkBackground = "#0d0d0d";
constexpr const char *PopupBackground = "#1e1e1e";
constexpr const char *DisabledBackground = "#444444"; // Disabled indicator backgrounds

// App Accent Color
constexpr const char *AccentAmber = "#FFB000"; // TX indicators, labels, highlights

// VFO Theme Colors (square, passband, markers, overlays)
constexpr const char *VfoACyan = "#00BFFF";  // VFO A: cyan theme
constexpr const char *VfoBGreen = "#00FF00"; // VFO B: green theme

// Status Colors
constexpr const char *TxRed = "#FF0000";
constexpr const char *AgcGreen = "#00FF00";
constexpr const char *RitCyan = "#00CED1";

// Text Colors
constexpr const char *TextWhite = "#FFFFFF";
constexpr const char *TextDark = "#333333";
constexpr const char *TextGray = "#999999";
constexpr const char *TextFaded = "#808080"; // Faded text (e.g., auto mode values)
constexpr const char *InactiveGray = "#666666";

// =============================================================================
// Button Gradient Colors
// =============================================================================

// Normal state gradients
constexpr const char *GradientTop = "#4a4a4a";
constexpr const char *GradientMid1 = "#3a3a3a";
constexpr const char *GradientMid2 = "#353535";
constexpr const char *GradientBottom = "#2a2a2a";

// Hover state gradients
constexpr const char *HoverTop = "#5a5a5a";
constexpr const char *HoverMid1 = "#4a4a4a";
constexpr const char *HoverMid2 = "#454545";
constexpr const char *HoverBottom = "#3a3a3a";

// Lighter button gradients (for TX function buttons, REC/STORE/RCL)
constexpr const char *LightGradientTop = "#888888";
constexpr const char *LightGradientMid1 = "#777777";
constexpr const char *LightGradientMid2 = "#6a6a6a";
constexpr const char *LightGradientBottom = "#606060";

// Selected button gradients
constexpr const char *SelectedTop = "#E0E0E0";
constexpr const char *SelectedMid1 = "#D0D0D0";
constexpr const char *SelectedMid2 = "#C8C8C8";
constexpr const char *SelectedBottom = "#B8B8B8";

// Borders
constexpr const char *BorderNormal = "#606060";
constexpr const char *BorderHover = "#808080";
constexpr const char *BorderPressed = "#909090";
constexpr const char *BorderSelected = "#AAAAAA";
constexpr const char *BorderLight = "#909090";
} // namespace Colors

namespace Dimensions {
// =============================================================================
// Border & Radius
// =============================================================================
constexpr int BorderWidth = 2;
constexpr int BorderRadius = 6;
constexpr int BorderRadiusLarge = 8;

// =============================================================================
// Shadow (for popup widgets)
// =============================================================================
constexpr int ShadowRadius = 16;
constexpr int ShadowOffsetX = 2;
constexpr int ShadowOffsetY = 4;
constexpr int ShadowMargin = ShadowRadius + 4; // 20px
constexpr int ShadowLayers = 8;

// =============================================================================
// Button Heights
// =============================================================================
constexpr int ButtonHeightLarge = 44;  // Menu overlay nav buttons
constexpr int ButtonHeightMedium = 36; // Bottom menu bar, popup buttons
constexpr int ButtonHeightSmall = 28;  // Function buttons (side panels)
constexpr int ButtonHeightMini = 24;   // Memory buttons (M1-M4, REC, etc.)

// =============================================================================
// Popup Layout
// =============================================================================
constexpr int PopupButtonWidth = 70;
constexpr int PopupButtonHeight = 44;
constexpr int PopupButtonSpacing = 8;
constexpr int MenuBarButtonWidth = 90; // Bottom menu bar buttons (fits "MAIN RX")
constexpr int PopupContentMargin = 12;

// =============================================================================
// Common UI Heights
// =============================================================================
constexpr int SeparatorHeight = 1;  // Horizontal/vertical separator lines
constexpr int MenuItemHeight = 40;  // Menu overlay items, frequency labels

// =============================================================================
// Common UI Widths
// =============================================================================
constexpr int FormLabelWidth = 80;  // Form field labels in dialogs
constexpr int VfoSquareSize = 45;   // VFO A/B indicator squares and mode labels
constexpr int NavButtonWidth = 54;  // Navigation buttons in overlays

// =============================================================================
// Font Sizes (in points) - use with QFont::setPointSize()
// =============================================================================
constexpr int FontSizeMicro = 6;   // Meter scale numbers
constexpr int FontSizeTiny = 7;    // Sub-labels (BANK, AF REC, MESSAGE)
constexpr int FontSizeSmall = 8;   // Scale fonts, secondary text
constexpr int FontSizeNormal = 9;  // Alt/secondary button text
constexpr int FontSizeMedium = 10; // Labels, descriptions
constexpr int FontSizeLarge = 11;  // Feature labels, primary labels
constexpr int FontSizeButton = 12; // Button text, value displays
constexpr int FontSizePopup = 14;  // Notifications, popup titles
constexpr int FontSizeTitle = 16;  // Large control buttons (+/-)
} // namespace Dimensions

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * @brief Draw a soft drop shadow behind popup content.
 *
 * Uses 8-layer blur technique for smooth shadow appearance.
 * Call this in paintEvent() before drawing content.
 *
 * @param painter The painter to draw with (should have NoPen set)
 * @param contentRect The rectangle of the popup content (not including shadow margin)
 * @param cornerRadius Corner radius for the shadow rounded rect
 */
void drawDropShadow(QPainter &painter, const QRect &contentRect, int cornerRadius = 8);

/**
 * @brief Create a standard button gradient for custom-painted widgets.
 *
 * @param top Y coordinate of gradient start
 * @param bottom Y coordinate of gradient end
 * @param hovered Whether button is in hover state
 * @return QLinearGradient configured with proper color stops
 */
QLinearGradient buttonGradient(int top, int bottom, bool hovered = false);

/**
 * @brief Create a selected/active button gradient.
 *
 * @param top Y coordinate of gradient start
 * @param bottom Y coordinate of gradient end
 * @return QLinearGradient configured with light/selected colors
 */
QLinearGradient selectedGradient(int top, int bottom);

/**
 * @brief Get the standard border color for buttons.
 * @return QColor for button borders
 */
QColor borderColor();

/**
 * @brief Get the selected/active border color.
 * @return QColor for selected button borders
 */
QColor borderColorSelected();

} // namespace K4Styles

#endif // K4STYLES_H
