#ifndef FEATUREMENUBAR_H
#define FEATUREMENUBAR_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

class FeatureMenuBar : public QWidget {
    Q_OBJECT

public:
    enum Feature { Attenuator, NbLevel, NrAdjust, ManualNotch };

    explicit FeatureMenuBar(QWidget *parent = nullptr);

    void showForFeature(Feature feature);
    void hideMenu();
    Feature currentFeature() const { return m_currentFeature; }
    bool isMenuVisible() const { return isVisible(); }

    // State updates (for Phase 2 RadioState integration)
    void setFeatureEnabled(bool enabled);
    void setValue(int value);
    void setValueUnit(const QString &unit);
    void setNbFilter(int filter); // 0=NONE, 1=NARROW, 2=WIDE

signals:
    void toggleRequested();
    void incrementRequested();
    void decrementRequested();
    void extraButtonClicked();
    void closeRequested();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUi();
    void updateForFeature();
    QString buttonStyle() const;
    QString buttonStyleSmall() const;

    QLabel *m_titleLabel;
    QPushButton *m_toggleBtn;
    QPushButton *m_extraBtn;
    QLabel *m_valueLabel;
    QPushButton *m_decrementBtn;
    QPushButton *m_incrementBtn;
    QLabel *m_encoderIcon;
    QLabel *m_encoderLabel;
    QPushButton *m_closeBtn;

    Feature m_currentFeature = Attenuator;
    bool m_featureEnabled = false;
    int m_value = 0;
    QString m_valueUnit;
    int m_nbFilter = 0; // 0=NONE, 1=NARROW, 2=WIDE
};

#endif // FEATUREMENUBAR_H
