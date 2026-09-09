#ifndef DASHBOARDTAB_H
#define DASHBOARDTAB_H

#include <QWidget>
#include <QTableWidget>
#include <QProgressBar>
#include <QLabel>
#include "../core/KeyPoolManager.h"

class DashboardTab : public QWidget {
    Q_OBJECT

public:
    explicit DashboardTab(KeyPoolManager *poolMgr, QWidget *parent = nullptr);

public slots:
    void refreshMetrics();
    void addLogEntry(const QString &time, const QString &clientIp, const QString &endpoint, const QString &provider, const QString &keyAlias, const QString &status, const QString &latency);

private:
    void setupUi();
    QWidget* createMetricCard(const QString &title, const QString &value, const QString &subtext);
    QWidget* createProviderCard(const QString &providerName, const QString &activeKeysStr, int rpmPct, const QString &statusText);
    
    KeyPoolManager *m_poolMgr;
    QLabel *m_lblActiveKeysVal;
    QLabel *m_lblRpmVal;
    QTableWidget *m_logTable;
};

#endif // DASHBOARDTAB_H
