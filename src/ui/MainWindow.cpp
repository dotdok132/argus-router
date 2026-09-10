#include "MainWindow.h"
#include "DashboardTab.h"
#include "ChatTab.h"
#include "KeyManagerTab.h"
#include "MemoryTab.h"
#include "SettingsTab.h"
#include "Styles.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    m_poolMgr = new KeyPoolManager(this);
    m_proxyServer = new HttpProxyServer(m_poolMgr, this);
    m_memMgr = new MemoryManager(this);
    m_proxyServer->setMemoryManager(m_memMgr);

    setWindowTitle("Argus Token Router");
    resize(1050, 680);

    // Apply Minimalist IDE QSS Theme
    setStyleSheet(Styles::darkTheme());

    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Top Header Bar
    QWidget *headerBar = new QWidget();
    headerBar->setObjectName("HeaderBar");
    QHBoxLayout *headerLayout = new QHBoxLayout(headerBar);
    headerLayout->setContentsMargins(14, 8, 14, 8);

    QLabel *lblTitle = new QLabel("ARGUS TOKEN ROUTER");
    lblTitle->setObjectName("AppTitle");

    QLabel *lblStatus = new QLabel("PROXY ACTIVE (127.0.0.1:8080)");
    lblStatus->setObjectName("StatusBadge");

    headerLayout->addWidget(lblTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(lblStatus);

    mainLayout->addWidget(headerBar);

    // Main Tabs Widget (Overview, AI Chat, Key Pool, Memory (.md), Settings)
    m_tabs = new QTabWidget(this);
    m_tabs->setContentsMargins(8, 8, 8, 8);

    DashboardTab *dashTab = new DashboardTab(m_poolMgr, this);
    connect(m_proxyServer, &HttpProxyServer::logTraffic, dashTab, &DashboardTab::addLogEntry);

    m_tabs->addTab(dashTab, "Overview");
    m_tabs->addTab(new ChatTab(m_poolMgr, this), "AI Chat");
    m_tabs->addTab(new KeyManagerTab(m_poolMgr, this), "Key Pool");
    m_tabs->addTab(new MemoryTab(m_memMgr, this), "Memory (.md)");
    m_tabs->addTab(new SettingsTab(this), "Settings");

    mainLayout->addWidget(m_tabs);

    setCentralWidget(central);

    // Start Local HTTP Proxy on Port 8080
    bool proxyOk = m_proxyServer->start(8080);
    if (proxyOk) {
        statusBar()->showMessage("Proxy listening on http://127.0.0.1:8080 • Ready for LLM requests");
    } else {
        lblStatus->setText("PROXY ERROR (Port 8080 Busy)");
        lblStatus->setStyleSheet("background-color: #481a1a; border: 1px solid #702626; color: #f14c4c;");
        statusBar()->showMessage("Error: Could not bind to port 8080");
    }
}
