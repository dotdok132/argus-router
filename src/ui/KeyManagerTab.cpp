#include "KeyManagerTab.h"
#include "AddKeyDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QComboBox>

KeyManagerTab::KeyManagerTab(KeyPoolManager *poolMgr, QWidget *parent)
    : QWidget(parent), m_poolMgr(poolMgr) {
    setupUi();
    connect(m_poolMgr, &KeyPoolManager::keysUpdated, this, &KeyManagerTab::refreshTable);
    refreshTable();
}

void KeyManagerTab::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Top Toolbar
    QHBoxLayout *toolbar = new QHBoxLayout();
    toolbar->setSpacing(8);
    
    QPushButton *btnAdd = new QPushButton("+ Add API Key");
    btnAdd->setObjectName("PrimaryButton");
    connect(btnAdd, &QPushButton::clicked, this, &KeyManagerTab::onAddKeyClicked);

    QPushButton *btnCheckPing = new QPushButton("Check Key Status");
    connect(btnCheckPing, &QPushButton::clicked, this, [this]() {
        if (m_poolMgr) m_poolMgr->testAllKeys();
    });

    QPushButton *btnResetPenalties = new QPushButton("Clear Penalties");
    connect(btnResetPenalties, &QPushButton::clicked, this, [this]() {
        if (m_poolMgr) {
            for (const auto &k : m_poolMgr->getKeys()) {
                m_poolMgr->toggleKey(k.id, true);
            }
        }
    });

    toolbar->addWidget(btnAdd);
    toolbar->addWidget(btnCheckPing);
    toolbar->addWidget(btnResetPenalties);
    toolbar->addStretch();

    QLabel *lblQueue = new QLabel("Failover Strategy:");
    lblQueue->setStyleSheet("color: #858585; font-size: 11px; font-weight: 600;");
    
    QComboBox *comboStrat = new QComboBox();
    comboStrat->addItem("Sequential Priority Queue");
    comboStrat->addItem("Round-Robin Rotation");
    comboStrat->addItem("Least Loaded Capacity");
    comboStrat->setFixedWidth(190);
    if (m_poolMgr) {
        comboStrat->setCurrentIndex(static_cast<int>(m_poolMgr->getQueueStrategy()));
    }
    connect(comboStrat, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        if (m_poolMgr) {
            m_poolMgr->setQueueStrategy(static_cast<QueueStrategy>(idx));
        }
    });

    toolbar->addWidget(lblQueue);
    toolbar->addWidget(comboStrat);

    mainLayout->addLayout(toolbar);

    // Keys Table Group
    QGroupBox *grpKeys = new QGroupBox("API Key Pool Configuration & Failover Order");
    QVBoxLayout *tableLayout = new QVBoxLayout(grpKeys);
    tableLayout->setContentsMargins(10, 14, 10, 10);

    m_keysTable = new QTableWidget(0, 8);
    QStringList headers = {"Provider", "Key Alias", "Key Mask", "Status", "RPM Limit", "TPM Limit", "Priority", "Actions"};
    m_keysTable->setHorizontalHeaderLabels(headers);
    
    // Hide unnecessary vertical row-number header and set generous row height
    m_keysTable->verticalHeader()->setVisible(false);
    m_keysTable->verticalHeader()->setDefaultSectionSize(42);

    // Precise Column Resize Modes
    QHeaderView *hdr = m_keysTable->horizontalHeader();
    hdr->setStretchLastSection(false);
    hdr->setSectionResizeMode(0, QHeaderView::ResizeToContents); // Provider
    hdr->setSectionResizeMode(1, QHeaderView::Stretch);          // Key Alias (takes remaining space)
    hdr->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Key Mask
    hdr->setSectionResizeMode(3, QHeaderView::ResizeToContents); // Status
    hdr->setSectionResizeMode(4, QHeaderView::ResizeToContents); // RPM Limit
    hdr->setSectionResizeMode(5, QHeaderView::ResizeToContents); // TPM Limit
    hdr->setSectionResizeMode(6, QHeaderView::ResizeToContents); // Priority
    hdr->setSectionResizeMode(7, QHeaderView::Fixed);            // Actions column (fixed width for buttons)
    m_keysTable->setColumnWidth(7, 300);

    m_keysTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_keysTable->setAlternatingRowColors(true);

    tableLayout->addWidget(m_keysTable);
    mainLayout->addWidget(grpKeys);
}

void KeyManagerTab::onAddKeyClicked() {
    AddKeyDialog dlg(m_poolMgr, this);
    if (dlg.exec() == QDialog::Accepted) {
        ApiKeyItem item = dlg.getApiKeyItem();
        if (m_poolMgr) {
            m_poolMgr->addKey(item);
        }
    }
}

void KeyManagerTab::refreshTable() {
    if (!m_poolMgr) return;
    m_keysTable->setRowCount(0);

    const auto &keys = m_poolMgr->getKeys();
    int totalKeys = keys.size();

    for (int r = 0; r < totalKeys; ++r) {
        const auto &k = keys[r];
        m_keysTable->insertRow(r);

        m_keysTable->setItem(r, 0, new QTableWidgetItem(k.provider));

        QString aliasText = k.isDuplicate ? QString("%1 [DUPLICATE]").arg(k.alias) : k.alias;
        QTableWidgetItem *aliasItem = new QTableWidgetItem(aliasText);
        if (k.isDuplicate) {
            aliasItem->setForeground(QColor("#ce9178"));
            aliasItem->setToolTip("Warning: Duplicate API Key detected in pool!");
        }
        m_keysTable->setItem(r, 1, aliasItem);

        QString mask = k.key.length() > 8 ? k.key.left(6) + "..." + k.key.right(3) : "********";
        QTableWidgetItem *maskItem = new QTableWidgetItem(mask);
        if (k.isDuplicate) {
            maskItem->setForeground(QColor("#ce9178"));
            maskItem->setToolTip("Warning: Duplicate API Key detected in pool!");
        }
        m_keysTable->setItem(r, 2, maskItem);

        QTableWidgetItem *statusItem = new QTableWidgetItem(k.status);
        if (k.status.contains("Active")) {
            statusItem->setForeground(QColor("#4ec9b0"));
        } else if (k.status.contains("Rate") || k.status.contains("Cooldown")) {
            statusItem->setForeground(QColor("#ce9178"));
        } else if (k.status.contains("Invalid") || k.status.contains("Error")) {
            statusItem->setForeground(QColor("#f14c4c"));
        } else {
            statusItem->setForeground(QColor("#858585"));
        }
        m_keysTable->setItem(r, 3, statusItem);

        m_keysTable->setItem(r, 4, new QTableWidgetItem(QString("%1 RPM").arg(k.rpmLimit)));
        m_keysTable->setItem(r, 5, new QTableWidgetItem(QString("%1 TPM").arg(k.tpmLimit)));
        m_keysTable->setItem(r, 6, new QTableWidgetItem(k.priority));

        // Tint duplicate rows amber
        if (k.isDuplicate) {
            QColor dupBg(55, 40, 25);
            for (int col = 0; col < 7; ++col) {
                if (auto *item = m_keysTable->item(r, col)) {
                    item->setBackground(dupBg);
                }
            }
        }

        // Action Cell Widget with Queue Move Up/Down, Test, Disable and Delete Buttons
        QWidget *actionWidget = new QWidget();
        actionWidget->setObjectName("ActionCellWidget");
        QHBoxLayout *actLayout = new QHBoxLayout(actionWidget);
        actLayout->setContentsMargins(4, 4, 4, 4);
        actLayout->setSpacing(4);

        QPushButton *btnUp = new QPushButton("^");
        btnUp->setObjectName("TableGhostButton");
        btnUp->setFixedWidth(24);
        btnUp->setToolTip("Move key UP in failover queue");
        if (r == 0) btnUp->setEnabled(false);

        QPushButton *btnDown = new QPushButton("v");
        btnDown->setObjectName("TableGhostButton");
        btnDown->setFixedWidth(24);
        btnDown->setToolTip("Move key DOWN in failover queue");
        if (r == totalKeys - 1) btnDown->setEnabled(false);

        QPushButton *btnTest = new QPushButton("Test");
        btnTest->setObjectName("TableGhostAccent");
        btnTest->setCursor(Qt::PointingHandCursor);
        btnTest->setToolTip("Send live validation ping to API endpoint");

        QPushButton *btnToggle = new QPushButton(k.enabled ? "Disable" : "Enable");
        btnToggle->setObjectName("TableGhostButton");
        btnToggle->setCursor(Qt::PointingHandCursor);

        QPushButton *btnDelete = new QPushButton("Delete");
        btnDelete->setObjectName("TableGhostDanger");
        btnDelete->setCursor(Qt::PointingHandCursor);

        QString keyId = k.id;
        connect(btnUp, &QPushButton::clicked, this, [this, r]() {
            if (m_poolMgr) m_poolMgr->moveKeyUp(r);
        });

        connect(btnDown, &QPushButton::clicked, this, [this, r]() {
            if (m_poolMgr) m_poolMgr->moveKeyDown(r);
        });

        connect(btnTest, &QPushButton::clicked, this, [this, keyId]() {
            if (m_poolMgr) m_poolMgr->testKey(keyId);
        });

        connect(btnToggle, &QPushButton::clicked, this, [this, keyId, k]() {
            if (m_poolMgr) m_poolMgr->toggleKey(keyId, !k.enabled);
        });

        connect(btnDelete, &QPushButton::clicked, this, [this, keyId]() {
            if (m_poolMgr) m_poolMgr->removeKey(keyId);
        });

        actLayout->addWidget(btnUp);
        actLayout->addWidget(btnDown);
        actLayout->addWidget(btnTest);
        actLayout->addWidget(btnToggle);
        actLayout->addWidget(btnDelete);

        m_keysTable->setCellWidget(r, 7, actionWidget);
    }
}
