#ifndef SMETERWIDGET_H
#define SMETERWIDGET_H

#include <QWidget>
#include <QColor>
#include <QTimer>

class SMeterWidget : public QWidget {
    Q_OBJECT

public:
    explicit SMeterWidget(QWidget *parent = nullptr);

    void setValue(double sValue); // S-units (0-9, then 9+dB)
    void setColor(const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void decayPeak();

private:
    double m_value = 0.0;
    double m_peakValue = 0.0;
    QColor m_color;
    QTimer *m_peakDecayTimer;
    static constexpr int PeakDecayIntervalMs = 50; // Timer interval
    static constexpr double PeakDecayRate = 0.1;   // Units per interval (~500ms full decay)
};

#endif // SMETERWIDGET_H
