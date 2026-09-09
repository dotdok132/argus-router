#ifndef KEYMANAGERTAB_H
#define KEYMANAGERTAB_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include "../core/KeyPoolManager.h"

class KeyManagerTab : public QWidget {
    Q_OBJECT

public:
    explicit KeyManagerTab(KeyPoolManager *poolMgr, QWidget *parent = nullptr);

private slots:
    void refreshTable();
    void onAddKeyClicked();

private:
    void setupUi();

    KeyPoolManager *m_poolMgr;
    QTableWidget *m_keysTable;
};

#endif // KEYMANAGERTAB_H
