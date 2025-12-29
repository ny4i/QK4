#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <QWidget>
#include <QVector>

class SpectrumWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SpectrumWidget(QWidget *parent = nullptr);

    void setData(const QVector<float> &spectrum);
    void setFrequencyRange(double startHz, double endHz);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<float> m_spectrum;
    double m_startFreq;
    double m_endFreq;
};

#endif // SPECTRUM_H
