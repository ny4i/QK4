#ifndef FREQUENCYDISPLAYWIDGET_H
#define FREQUENCYDISPLAYWIDGET_H

#include <QWidget>
#include <QColor>
#include <QFont>

/**
 * FrequencyDisplayWidget - Inline frequency display with segment-based editing.
 *
 * Features:
 * - Custom-painted frequency with dot separators (XX.XXX.XXX)
 * - Click any digit to enter edit mode at that position
 * - All digits change to edit color (VFO theme: cyan for A, green for B)
 * - Type digits to replace at cursor position (auto-advances)
 * - Arrow keys to navigate between digits
 * - Enter to send frequency, Escape/click-outside to cancel
 *
 * Usage:
 *   auto *freq = new FrequencyDisplayWidget(parent);
 *   freq->setEditModeColor(QColor("#00BFFF")); // Cyan for VFO A
 *   freq->setFrequency("7.024.980");
 *   connect(freq, &FrequencyDisplayWidget::frequencyEntered, ...);
 */
class FrequencyDisplayWidget : public QWidget {
    Q_OBJECT

public:
    explicit FrequencyDisplayWidget(QWidget *parent = nullptr);

    // Set the displayed frequency (with or without dots)
    // Examples: "7.024.980", "7024980", "14.024.980"
    void setFrequency(const QString &frequency);

    // Get the current frequency as plain digits (no dots)
    QString frequency() const;

    // Get the displayed text with dots (e.g., "7.024.980")
    QString displayText() const;

    // Set the color used when in edit mode (typically VFO theme color)
    void setEditModeColor(const QColor &color);

    // Set the color used when NOT in edit mode (normally white, gray when dimmed)
    void setNormalColor(const QColor &color);

    // Check if currently in edit mode
    bool isEditing() const;

signals:
    // Emitted when user presses Enter to confirm frequency entry
    // digits is the frequency as plain digits (e.g., "7024980")
    void frequencyEntered(const QString &digits);

    // Emitted when editing is cancelled (Escape or click outside)
    void editingCancelled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    // Enter edit mode with cursor at the specified digit position (0-7)
    void enterEditMode(int digitPosition);

    // Exit edit mode, optionally sending the frequency
    void exitEditMode(bool send);

    // Convert mouse X coordinate to digit position (0-7), or -1 if not on a digit
    int digitPositionFromX(int x) const;

    // Get the bounding rectangle for a character at the given index
    QRect charRectAt(int charIndex) const;

    // Convert character index in display string to digit index (0-7)
    // Returns -1 for dot characters
    int digitIndexFromCharIndex(int charIndex) const;

    // Format m_digits as display string with dots (e.g., "7.024.980")
    QString formatWithDots() const;

    // Parse frequency string and normalize to 8 digits
    void parseFrequency(const QString &freq);

    // Member variables
    QString m_digits;          // 8-digit string, left-padded with zeros
    QString m_originalDigits;  // Backup for cancel operation
    int m_cursorPosition = -1; // -1 = not editing, 0-7 = digit position

    QColor m_normalColor; // White - normal display color
    QColor m_editColor;   // Cyan/Green - edit mode color
    QFont m_font;         // JetBrains Mono, 32px bold

    // Cached character metrics for click detection
    int m_charWidth = 0;
    int m_dotWidth = 0;
};

#endif // FREQUENCYDISPLAYWIDGET_H
