#ifndef K4STYLES_H
#define K4STYLES_H

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
// Button gradients
constexpr const char *GradientTop = "#4a4a4a";
constexpr const char *GradientMid1 = "#3a3a3a";
constexpr const char *GradientMid2 = "#353535";
constexpr const char *GradientBottom = "#2a2a2a";

// Hover gradients
constexpr const char *HoverTop = "#5a5a5a";
constexpr const char *HoverMid1 = "#4a4a4a";
constexpr const char *HoverMid2 = "#454545";
constexpr const char *HoverBottom = "#3a3a3a";

// Borders
constexpr const char *BorderNormal = "#606060";
constexpr const char *BorderHover = "#808080";
constexpr const char *BorderPressed = "#909090";
constexpr const char *BorderSelected = "#AAAAAA";

// Text
constexpr const char *TextWhite = "#FFFFFF";
constexpr const char *TextDark = "#333333";
constexpr const char *TextGray = "#666666";

// Selected button
constexpr const char *SelectedTop = "#E0E0E0";
constexpr const char *SelectedMid1 = "#D0D0D0";
constexpr const char *SelectedMid2 = "#C8C8C8";
constexpr const char *SelectedBottom = "#B8B8B8";

// Popup
constexpr const char *PopupBackground = "#1e1e1e";
constexpr const char *IndicatorStrip = "#555555";
} // namespace Colors

namespace Dimensions {
constexpr int BorderWidth = 2;
constexpr int BorderRadius = 6;
constexpr int BorderRadiusLarge = 8;

// Shadow
constexpr int ShadowRadius = 16;
constexpr int ShadowOffsetX = 2;
constexpr int ShadowOffsetY = 4;
constexpr int ShadowMargin = ShadowRadius + 4; // 20px
} // namespace Dimensions

} // namespace K4Styles

#endif // K4STYLES_H
