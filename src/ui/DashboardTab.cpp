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
        m_lblActiveKeysSub = lblSub;
    } else if (title.contains("Session Tokens")) {
        m_lblSessionTokensVal = lblVal;
    } else if (title.contains("RPM")) {
        m_lblRpmVal = lblVal;
    } else if (title.contains("TPM")) {
        m_lblTpmVal = lblVal;
    } else if (title.contains("Rate Limits")) {
        m_lblFailoversVal = lblVal;
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
    if (statusText == "ACTIVE") {
        lblStatus->setStyleSheet("font-size: 10px; font-weight: 600; color: #4ec9b0; background: #1b382b; padding: 2px 5px; border-radius: 3px; font-family: monospace;");
    } else {
        lblStatus->setStyleSheet("font-size: 10px; font-weight: 600; color: #858585; background: #2d2d2d; padding: 2px 5px; border-radius: 3px; font-family: monospace;");
    }

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

    metricsLayout->addWidget(createMetricCard("Total Pool RPM", "0 RPM", "0% Total Load"));
    metricsLayout->addWidget(createMetricCard("Session Tokens Used", "0 Tokens", "Live Session Tracker"));
    metricsLayout->addWidget(createMetricCard("Active Keys", "0 Active", "0 Providers"));
    metricsLayout->addWidget(createMetricCard("Rate Limits Handled", "0 Failovers", "0 Client Downtime"));

    mainLayout->addLayout(metricsLayout);

    // Provider Capacity Section
    QGroupBox *grpProviders = new QGroupBox("Provider Pool Capacity");
    QVBoxLayout *provLayout = new QVBoxLayout(grpProviders);
    provLayout->setContentsMargins(10, 14, 10, 10);
    provLayout->setSpacing(8);

    m_provCardsLayout = new QHBoxLayout();
    m_provCardsLayout->setSpacing(8);

    provLayout->addLayout(m_provCardsLayout);
    mainLayout->addWidget(grpProviders);

    // Live Request Traffic Table
    QGroupBox *grpLog = new QGroupBox("HTTP Traffic Log (OpenAI API Endpoint /v1/chat/completions)");
    QVBoxLayout *logLayout = new QVBoxLayout(grpLog);
    logLayout->setContentsMargins(10, 14, 10, 10);

    m_logTable = new QTableWidget(0, 8);
    QStringList headers = {"Time", "Client IP", "Endpoint / Model", "Provider", "Key Selected", "Status", "Tokens", "Latency"};
    m_logTable->setHorizontalHeaderLabels(headers);
    m_logTable->verticalHeader()->setVisible(false);
    m_logTable->verticalHeader()->setDefaultSectionSize(34);

    QHeaderView *hdr = m_logTable->horizontalHeader();
    hdr->setStretchLastSection(false);
    hdr->setSectionResizeMode(0, QHeaderView::ResizeToContents); // Time
    hdr->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Client IP
    hdr->setSectionResizeMode(2, QHeaderView::Stretch);          // Endpoint / Model
    hdr->setSectionResizeMode(3, QHeaderView::ResizeToContents); // Provider
    hdr->setSectionResizeMode(4, QHeaderView::ResizeToContents); // Key Selected
    hdr->setSectionResizeMode(5, QHeaderView::ResizeToContents); // Status
    hdr->setSectionResizeMode(6, QHeaderView::ResizeToContents); // Tokens
    hdr->setSectionResizeMode(7, QHeaderView::ResizeToContents); // Latency

    m_logTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_logTable->setAlternatingRowColors(true);

    logLayout->addWidget(m_logTable);
    mainLayout->addWidget(grpLog);
}

void DashboardTab::refreshMetrics() {
    if (!m_poolMgr) return;

    if (m_provCardsLayout) {
        QLayoutItem *child;
        while ((child = m_provCardsLayout->takeAt(0)) != nullptr) {
            if (child->widget()) {
                child->widget()->deleteLater();
            }
            delete child;
        }
    }

    const auto &keys = m_poolMgr->getKeys();

    int activeCount = 0;
    int totalRpm = 0;
    int totalTpm = 0;
    qint64 totalServed = m_poolMgr->getTotalTokensServed();

    struct ProvInfo {
        int totalKeys = 0;
        int activeKeys = 0;
        int totalRpm = 0;
    };
    QMap<QString, ProvInfo> provMap;

    for (const auto &k : keys) {
        QString p = k.provider.isEmpty() ? "Custom" : k.provider;
        if (p.toLower() == "gemini") p = "Google Gemini";
        else if (p.toLower() == "openrouter") p = "OpenRouter";
        else if (p.toLower() == "groq") p = "Groq";
        else if (p.toLower() == "anthropic") p = "Anthropic";

        int effRpm = k.rpmLimit > 0 ? k.rpmLimit : (p.contains("Gemini") ? 15 : (p.contains("OpenRouter") ? 20 : (p.contains("Groq") ? 30 : 60)));
        int effTpm = k.tpmLimit > 0 ? k.tpmLimit : (p.contains("Gemini") ? 1000000 : (p.contains("OpenRouter") ? 200000 : (p.contains("Groq") ? 100000 : 500000)));

        provMap[p].totalKeys++;
        if (k.enabled) {
            provMap[p].activeKeys++;
            provMap[p].totalRpm += effRpm;
            activeCount++;
            totalRpm += effRpm;
            totalTpm += effTpm;
        }
    }

    if (m_lblActiveKeysVal) {
        m_lblActiveKeysVal->setText(QString("%1 Active").arg(activeCount));
    }
    if (m_lblActiveKeysSub) {
        m_lblActiveKeysSub->setText(QString("%1 Providers").arg(provMap.size()));
    }
    if (m_lblRpmVal) {
        m_lblRpmVal->setText(QString("%1 RPM").arg(totalRpm));
    }
    if (m_lblTpmVal) {
        if (totalServed > 0) {
            m_lblTpmVal->setText(QString("%1 Served").arg(QLocale().toString(totalServed)));
        } else {
            m_lblTpmVal->setText(QLocale().toString(totalTpm));
        }
    }

    if (provMap.isEmpty()) {
        QLabel *emptyLabel = new QLabel("No API keys configured yet. Add your first API key in the Key Pool tab.");
        emptyLabel->setStyleSheet("color: #858585; font-size: 12px; font-style: italic; padding: 10px;");
        m_provCardsLayout->addWidget(emptyLabel);
    } else {
        for (auto it = provMap.constBegin(); it != provMap.constEnd(); ++it) {
            const QString &provName = it.key();
            const ProvInfo &info = it.value();
            QString statusText = info.activeKeys > 0 ? "ACTIVE" : "DISABLED";

            int rpmPct = info.totalRpm > 0 ? 100 : 0;
            QString keysSub = QString("%1 / %2 Keys Active").arg(info.activeKeys).arg(info.totalKeys);

            m_provCardsLayout->addWidget(createProviderCard(provName, keysSub, rpmPct, statusText));
        }
    }
}

void DashboardTab::addLogEntry(const QString &time, const QString &clientIp, const QString &endpoint, const QString &provider, const QString &keyAlias, const QString &status, const QString &latency, qint64 tokens) {
    if (!m_logTable) return;

    m_totalRequests++;
    if (tokens > 0) {
        m_totalTokensServed += tokens;
        m_sessionTokens += tokens;
        if (m_lblSessionTokensVal) {
            m_lblSessionTokensVal->setText(QString("%1 Tokens").arg(QLocale().toString(m_sessionTokens)));
        }
        if (m_lblTpmVal) {
            m_lblTpmVal->setText(QString("%1 Served").arg(QLocale().toString(m_totalTokensServed)));
        }
    }
    if (status.contains("Failover")) {
        m_totalFailovers++;
        if (m_lblFailoversVal) {
            m_lblFailoversVal->setText(QString("%1 Failovers").arg(m_totalFailovers));
        }
    }

    int row = 0;
    m_logTable->insertRow(row);

    m_logTable->setItem(row, 0, new QTableWidgetItem(time));
    m_logTable->setItem(row, 1, new QTableWidgetItem(clientIp));
    m_logTable->setItem(row, 2, new QTableWidgetItem(endpoint));
    m_logTable->setItem(row, 3, new QTableWidgetItem(provider));
    m_logTable->setItem(row, 4, new QTableWidgetItem(keyAlias));

    QTableWidgetItem *statusItem = new QTableWidgetItem(status);
    if (status.contains("Failover")) {
        statusItem->setForeground(QColor("#ce9178")); // Amber for automatic failover
        statusItem->setToolTip("Upstream provider returned an error. Router automatically failed over to the next key in pool!");
    } else if (status.contains("200") || status.contains("OK")) {
        statusItem->setForeground(QColor("#4ec9b0")); // Teal for success
    } else {
        statusItem->setForeground(QColor("#f14c4c")); // Red for unhandled error
    }
    m_logTable->setItem(row, 5, statusItem);

    QString tokStr = tokens > 0 ? QString("%1 tok").arg(QLocale().toString(tokens)) : "-";
    QTableWidgetItem *tokItem = new QTableWidgetItem(tokStr);
    tokItem->setForeground(QColor("#dcdcaa")); // Soft yellow for token counts
    m_logTable->setItem(row, 6, tokItem);

    m_logTable->setItem(row, 7, new QTableWidgetItem(latency));

    if (m_logTable->rowCount() > 50) {
        m_logTable->removeRow(50);
    }
}
