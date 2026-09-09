#ifndef MEMORYTAB_H
#define MEMORYTAB_H

#include <QWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include "../core/MemoryManager.h"

class MemoryTab : public QWidget {
    Q_OBJECT

public:
    explicit MemoryTab(MemoryManager *memMgr, QWidget *parent = nullptr);

public slots:
    void refreshMemoryList();
    void onMemorySelected(QListWidgetItem *current, QListWidgetItem *previous);
    void onSaveClicked();
    void onCreateClicked();
    void onDeleteClicked();

private:
    void setupUi();

    MemoryManager *m_memMgr;
    QListWidget *m_listWidget;
    QTextEdit *m_textEditor;
    QLabel *m_lblCurrentFile;
    QPushButton *m_btnSave;
    QString m_activeFileName;
};

#endif // MEMORYTAB_H
