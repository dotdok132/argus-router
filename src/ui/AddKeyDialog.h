#ifndef ADDKEYDIALOG_H
#define ADDKEYDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include "../core/KeyPoolManager.h"

class AddKeyDialog : public QDialog {
    Q_OBJECT

public:
    explicit AddKeyDialog(KeyPoolManager *poolMgr, QWidget *parent = nullptr);
    ApiKeyItem getApiKeyItem() const;

private slots:
    void checkDuplicateKey(const QString &text);

private:
    void setupUi();

    KeyPoolManager *m_poolMgr;
    QComboBox *m_comboProvider;
    QLineEdit *m_editAlias;
    QLineEdit *m_editKey;
    QSpinBox *m_spinRpm;
    QSpinBox *m_spinTpm;
    QComboBox *m_comboPriority;
    QLabel *m_lblWarning;
};

#endif // ADDKEYDIALOG_H
