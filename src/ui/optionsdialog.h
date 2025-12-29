#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>

class RadioState;
class KPA1500Client;

class OptionsDialog : public QDialog {
    Q_OBJECT

public:
    explicit OptionsDialog(RadioState *radioState, KPA1500Client *kpa1500Client, QWidget *parent = nullptr);
    ~OptionsDialog() = default;

private slots:
    void onKpa1500ConnectionStateChanged();

private:
    void setupUi();
    QWidget *createAboutPage();
    QWidget *createKpodPage();
    QWidget *createKpa1500Page();
    QWidget *createAudioInputPage();
    QWidget *createAudioOutputPage();
    QWidget *createNetworkPage();
    void updateKpa1500Status();

    RadioState *m_radioState;
    KPA1500Client *m_kpa1500Client;
    QListWidget *m_tabList;
    QStackedWidget *m_pageStack;
    QCheckBox *m_kpodEnableCheckbox;

    // KPA1500 settings
    QLabel *m_kpa1500StatusLabel;
    QLineEdit *m_kpa1500HostEdit;
    QLineEdit *m_kpa1500PortEdit;
    QLineEdit *m_kpa1500PollIntervalEdit;
    QCheckBox *m_kpa1500EnableCheckbox;
};

#endif // OPTIONSDIALOG_H
