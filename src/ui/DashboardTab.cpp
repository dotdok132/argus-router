#include "DashboardTab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QDateTime>

DashboardTab::DashboardTab(KeyPoolManager *poolMgr, QWidget *parent)
    : QWidget(parent), m_poolMgr(poolMgr) {
    setupUi();
    if (m_poolMgr) {
        connect(m_poolMgr, &KeyPoolManager::keysUpdated, this, &DashboardTab::refreshMetrics);
        refreshMetrics();
    }
}

QWidget* DashboardTab::createMetricCard(const QString &title, const QString &value, const QString &subtext) {
    QWidget *card = new QWidget();
    card->setStyleSheet(R"(
        QWidget {
            background-color: #252526;
            border: 1px solid #3c3c3c;
            border-radius: 4px;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(2);

    QLabel *lblTitle = new QLabel(title);
    lblTitle->setStyleSheet("color: #858585; font-size: 11px; font-weight: 600; text-transform: uppercase;");
    
    QLabel *lblVal = new QLabel(value);
    lblVal->setStyleSheet("color: #ffffff; font-size: 18px; font-weight: 700; font-family: monospace;");

    QLabel *lblSub = new QLabel(subtext);
    lblSub->setStyleSheet("color: #6e6e6e; font-size: 11px;");

    if (title.contains("Active Keys")) {
        m_lblActiveKeysVal = lblVal;
    } else if (title.contains("RPM")) {
        m_lblRpmVal = lblVal;
    }

    layout->addWidget(lblTitle);
    layout->addWidget(lblVal);
    layout->addWidget(lblSub);

    return card;
}

QWidget* DashboardTab::createProviderCard(const QString &providerName, const QString &activeKeysStr, int rpmPct, const QString &statusText) {
    QWidget *card = new QWidget();
    card->setStyleSheet(R"(
        QWidget {
            background-color: #252526;
            border: 1px solid #3c3c3c;
            border-radius: 4px;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(6);

    QHBoxLayout *topLayout = new QHBoxLayout();
    QLabel *lblName = new QLabel(providerName);
    lblName->setStyleSheet("font-size: 12px; font-weight: 600; color: #ffffff;");

    QLabel *lblStatus = new QLabel(statusText);
    lblStatus->setStyleSheet("font-size: 10px; font-weight: 600; color: #4ec9b0; background: #1b382b; padding: 2px 5px; border-radius: 3px; font-family: monospace;");

    topLayout->addWidget(lblName);
    topLayout->addStretch();
    topLayout->addWidget(lblStatus);

    QLabel *lblKeys = new QLabel(activeKeysStr);
    lblKeys->setStyleSheet("color: #858585; font-size: 11px;");

    QProgressBar *bar = new QProgressBar();
    bar->setValue(rpmPct);
    bar->setFormat(QString("Capacity: %1%").arg(rpmPct));

    layout->addLayout(topLayout);
    layout->addWidget(lblKeys);
    layout->addWidget(bar);

    return card;
}

void DashboardTab::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Metrics Row
    QHBoxLayout *metricsLayout = new QHBoxLayout();
    metricsLayout->setSpacing(8);

    metricsLayout->addWidget(createMetricCard("Total Pool RPM", "0 / 0", "0% Total Load"));
    metricsLayout->addWidget(createMetricCard("Total Pool TPM", "0", "Token Bucket Active"));
    metricsLayout->addWidget(createMetricCard("Active Keys", "0 Active", "0 Providers"));
    metricsLayout->addWidget(createMetricCard("Rate Limits Handled", "0 Rotations", "0 Client Downtime"));

    mainLayout->addLayout(metricsLayout);

    // Provider Capacity Section
    QGroupBox *grpProviders = new QGroupBox("Provider Pool Capacity");
    QVBoxLayout *provLayout = new QVBoxLayout(grpProviders);
    provLayout->setContentsMargins(10, 14, 10, 10);
    provLayout->setSpacing(8);

    QHBoxLayout *pCardsLayout = new QHBoxLayout();
    pCardsLayout->setSpacing(8);

    pCardsLayout->addWidget(createProviderCard("Google Gemini", "0 Active Keys", 0, "STANDBY"));
    pCardsLayout->addWidget(createProviderCard("Groq Speed Pool", "0 Active Keys", 0, "STANDBY"));
    pCardsLayout->addWidget(createProviderCard("OpenRouter Auto-Free", "0 Auto Pool", 0, "STANDBY"));

    provLayout->addLayout(pCardsLayout);
    mainLayout->addWidget(grpProviders);

    // Live Request Traffic Table
    QGroupBox *grpLog = new QGroupBox("HTTP Traffic Log (OpenAI API Endpoint /v1/chat/completions)");
    QVBoxLayout *logLayout = new QVBoxLayout(grpLog);
    logLayout->setContentsMargins(10, 14, 10, 10);

    m_logTable = new QTableWidget(0, 7);
    QStringList headers = {"Time", "Client IP", "Endpoint / Model", "Provider", "Key Selected", "Status", "Latency"};
    m_logTable->setHorizontalHeaderLabels(headers);
    m_logTable->verticalHeader()->setVisible(false);
    m_logTable->verticalHeader()->setDefaultSectionSize(34);
    m_logTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_logTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_logTable->setAlternatingRowColors(true);

    logLayout->addWidget(m_logTable);
    mainLayout->addWidget(grpLog);
}

void DashboardTab::refreshMetrics() {
    if (!m_poolMgr) return;
    const auto &keys = m_poolMgr->getKeys();

    int activeCount = 0;
    int totalRpm = 0;
    for (const auto &k : keys) {
        if (k.enabled) {
            activeCount++;
            totalRpm += k.rpmLimit;
        }
    }

    if (m_lblActiveKeysVal) {
        m_lblActiveKeysVal->setText(QString("%1 Active").arg(activeCount));
    }
    if (m_lblRpmVal) {
        m_lblRpmVal->setText(QString("0 / %1").arg(totalRpm));
    }
}

void DashboardTab::addLogEntry(const QString &time, const QString &clientIp, const QString &endpoint, const QString &provider, const QString &keyAlias, const QString &status, const QString &latency) {
    if (!m_logTable) return;

    int row = 0;
    m_logTable->insertRow(row);

    m_logTable->setItem(row, 0, new QTableWidgetItem(time));
    m_logTable->setItem(row, 1, new QTableWidgetItem(clientIp));
    m_logTable->setItem(row, 2, new QTableWidgetItem(endpoint));
    m_logTable->setItem(row, 3, new QTableWidgetItem(provider));
    m_logTable->setItem(row, 4, new QTableWidgetItem(keyAlias));

    QTableWidgetItem *statusItem = new QTableWidgetItem(status);
    if (status.contains("200")) {
        statusItem->setForeground(QColor("#4ec9b0"));
    } else {
        statusItem->setForeground(QColor("#f14c4c"));
    }
    m_logTable->setItem(row, 5, statusItem);
    m_logTable->setItem(row, 6, new QTableWidgetItem(latency));

    if (m_logTable->rowCount() > 50) {
        m_logTable->removeRow(50);
    }
}
