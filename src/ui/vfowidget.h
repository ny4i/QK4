#ifndef VFOWIDGET_H
#define VFOWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QColor>

class SMeterWidget;
class MiniPanRhiWidget;

class VFOWidget : public QWidget {
    Q_OBJECT

public:
    enum VFOType { VFO_A, VFO_B };

    explicit VFOWidget(VFOType type, QWidget *parent = nullptr);

    // Setters for radio state
    void setFrequency(const QString &formatted);
    void setTuningRate(int rate); // 0-5: 1Hz, 10Hz, 100Hz, 1kHz, 10kHz, 100kHz
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

    // Mini-pan configuration (applied when miniPan is created, or immediately if it exists)
    void setMiniPanMode(const QString &mode);
    void setMiniPanFilterBandwidth(int bw);
    void setMiniPanIfShift(int shift);
    void setMiniPanCwPitch(int pitch);
    void setMiniPanNotchFilter(bool enabled, int pitchHz);
    void setMiniPanSpectrumColor(const QColor &color);
    void setMiniPanPassbandColor(const QColor &color);

    // Access to mini-pan (may return nullptr if not yet created)
    MiniPanRhiWidget *miniPan() const { return m_miniPan; }

    // Get type
    VFOType type() const { return m_type; }

signals:
    void normalContentClicked(); // User clicked normal view → show mini-pan
    void miniPanClicked();       // User clicked mini-pan → show normal view

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUi();
    void drawTuningRateIndicator(QPainter &painter);

    VFOType m_type;
    QString m_primaryColor;
    QString m_inactiveColor;

    // Tuning rate indicator (0-5: 1Hz to 100kHz)
    int m_tuningRate = 3; // Default 1kHz

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
    MiniPanRhiWidget *m_miniPan;

    // Pending mini-pan configuration (applied when created)
    QString m_pendingMode;
    int m_pendingFilterBw = 2400;
    int m_pendingIfShift = 50;
    int m_pendingCwPitch = 600;
    bool m_pendingNotchEnabled = false;
    int m_pendingNotchPitchHz = 0;
    QColor m_pendingSpectrumColor;
    QColor m_pendingPassbandColor;
};

#endif // VFOWIDGET_H
