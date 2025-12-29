#ifndef RADIOMANAGERDIALOG_H
#define RADIOMANAGERDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include "settings/radiosettings.h"

class RadioManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RadioManagerDialog(QWidget *parent = nullptr);

    RadioEntry selectedRadio() const;
    bool hasSelection() const;

signals:
    void connectRequested(const RadioEntry &radio);

private slots:
    void onConnectClicked();
    void onNewClicked();
    void onSaveClicked();
    void onDeleteClicked();
    void onBackClicked();
    void onSelectionChanged();
    void onItemDoubleClicked(QListWidgetItem *item);
    void refreshList();

private:
    void setupUi();
    void updateButtonStates();
    void clearFields();
    void populateFieldsFromSelection();

    QListWidget *m_radioList;
    QLineEdit *m_nameEdit;
    QLineEdit *m_hostEdit;
    QLineEdit *m_portEdit;
    QLineEdit *m_passwordEdit;

    QPushButton *m_connectButton;
    QPushButton *m_newButton;
    QPushButton *m_saveButton;
    QPushButton *m_deleteButton;
    QPushButton *m_backButton;

    int m_currentIndex;
};

#endif // RADIOMANAGERDIALOG_H
