#ifndef VFOWIDGET_H
#define VFOWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QColor>

class SMeterWidget;
class MiniPanWidget;

class VFOWidget : public QWidget {
    Q_OBJECT

public:
    enum VFOType { VFO_A, VFO_B };

    explicit VFOWidget(VFOType type, QWidget *parent = nullptr);

    // Setters for radio state
    void setFrequency(const QString &formatted);
    void setSMeterValue(double value);
    void setAGC(const QString &mode);
    void setPreamp(bool on);
    void setAtt(bool on);
    void setNB(bool on);
    void setNR(bool on);
    void setNotch(bool autoEnabled, bool manualEnabled);

    // Mini-pan support
    void updateMiniPan(const QByteArray &data);
    void showMiniPan();
    void showNormal();
    bool isMiniPanVisible() const;

    // Access to mini-pan for direct data updates
    MiniPanWidget *miniPan() const { return m_miniPan; }

    // Get type
    VFOType type() const { return m_type; }

signals:
    void normalContentClicked(); // For mini-pan toggle

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupUi();

    VFOType m_type;
    QString m_primaryColor;
    QString m_inactiveColor;

    // Widgets
    QLabel *m_frequencyLabel;
    SMeterWidget *m_sMeter;
    QLabel *m_agcLabel;
    QLabel *m_preampLabel;
    QLabel *m_attLabel;
    QLabel *m_nbLabel;
    QLabel *m_nrLabel;
    QLabel *m_ntchLabel;

    // Stacked widget for normal/mini-pan toggle
    QStackedWidget *m_stackedWidget;
    QWidget *m_normalContent;
    MiniPanWidget *m_miniPan;
};

#endif // VFOWIDGET_H
