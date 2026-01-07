#ifndef FNPOPUPWIDGET_H
#define FNPOPUPWIDGET_H

#include <QWidget>
#include <QVector>

class QVBoxLayout;

// Function ID constants for macro system
namespace MacroIds {
// Programmable Function keys (K4 front panel)
const QString PF1 = "PF1";
const QString PF2 = "PF2";
const QString PF3 = "PF3";
const QString PF4 = "PF4";

// Fn popup functions
const QString FnF1 = "Fn.F1";
const QString FnF2 = "Fn.F2";
const QString FnF3 = "Fn.F3";
const QString FnF4 = "Fn.F4";
const QString FnF5 = "Fn.F5";
const QString FnF6 = "Fn.F6";
const QString FnF7 = "Fn.F7";
const QString FnF8 = "Fn.F8";

// Special buttons
const QString RemAnt = "REM_ANT";

// KPOD buttons (T=Tap, H=Hold)
const QString Kpod1T = "K-pod.1T";
const QString Kpod1H = "K-pod.1H";
const QString Kpod2T = "K-pod.2T";
const QString Kpod2H = "K-pod.2H";
const QString Kpod3T = "K-pod.3T";
const QString Kpod3H = "K-pod.3H";
const QString Kpod4T = "K-pod.4T";
const QString Kpod4H = "K-pod.4H";
const QString Kpod5T = "K-pod.5T";
const QString Kpod5H = "K-pod.5H";
const QString Kpod6T = "K-pod.6T";
const QString Kpod6H = "K-pod.6H";
const QString Kpod7T = "K-pod.7T";
const QString Kpod7H = "K-pod.7H";
const QString Kpod8T = "K-pod.8T";
const QString Kpod8H = "K-pod.8H";

// Built-in functions (not user-configurable)
const QString ScrnCap = "SCRN_CAP";
const QString Macros = "MACROS";
const QString SwList = "SW_LIST";
const QString Update = "UPDATE";
const QString DxList = "DXLIST";
} // namespace MacroIds

// Dual-action button similar to DisplayMenuButton
class FnMenuButton : public QWidget {
    Q_OBJECT

public:
    explicit FnMenuButton(const QString &primaryText, const QString &alternateText, QWidget *parent = nullptr);

    void setPrimaryText(const QString &text);
    void setAlternateText(const QString &text);
    QString primaryText() const { return m_primaryText; }
    QString alternateText() const { return m_alternateText; }

    void setPrimaryFunctionId(const QString &id) { m_primaryFunctionId = id; }
    void setAlternateFunctionId(const QString &id) { m_alternateFunctionId = id; }
    QString primaryFunctionId() const { return m_primaryFunctionId; }
    QString alternateFunctionId() const { return m_alternateFunctionId; }

signals:
    void clicked();      // Left click -> primary action
    void rightClicked(); // Right click -> secondary action

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QString m_primaryText;
    QString m_alternateText;
    QString m_primaryFunctionId;
    QString m_alternateFunctionId;
    bool m_hovered = false;
};

// Fn popup widget with 7 dual-action buttons
class FnPopupWidget : public QWidget {
    Q_OBJECT

public:
    explicit FnPopupWidget(QWidget *parent = nullptr);

    void showAboveButton(QWidget *triggerButton);
    void hidePopup();

    // Update button labels from macro settings
    void updateButtonLabels();

signals:
    void functionTriggered(const QString &functionId);
    void closed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupButtons();
    void onButtonClicked(int buttonIndex);
    void onButtonRightClicked(int buttonIndex);

    QVector<FnMenuButton *> m_buttons;
    int m_triangleOffset = 0;
};

#endif // FNPOPUPWIDGET_H
