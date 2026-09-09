#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>
#include "../core/KeyPoolManager.h"
#include "../core/HttpProxyServer.h"
#include "../core/MemoryManager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupHeader();
    void setupTabs();

    KeyPoolManager *m_poolMgr;
    HttpProxyServer *m_proxyServer;
    MemoryManager *m_memMgr;
    QTabWidget *m_tabs;
};

#endif // MAINWINDOW_H
