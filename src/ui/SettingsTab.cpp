#include "SettingsTab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>

SettingsTab::SettingsTab(QWidget *parent) : QWidget(parent) {
    setupUi();
}

void SettingsTab::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // 1. Server Configuration
    QGroupBox *grpProxy = new QGroupBox("Server Configuration (Local API Proxy)");
    QVBoxLayout *proxyLayout = new QVBoxLayout(grpProxy);
    proxyLayout->setContentsMargins(10, 14, 10, 10);
    proxyLayout->setSpacing(8);

    QHBoxLayout *hostLayout = new QHBoxLayout();
    QLabel *lblHost = new QLabel("Listen Address:");
    QLineEdit *editHost = new QLineEdit("127.0.0.1");
    editHost->setFixedWidth(160);

    QLabel *lblPort = new QLabel("Listen Port:");
    QSpinBox *spinPort = new QSpinBox();
    spinPort->setRange(1024, 65535);
    spinPort->setValue(8080);
    spinPort->setFixedWidth(100);

    hostLayout->addWidget(lblHost);
    hostLayout->addWidget(editHost);
    hostLayout->addSpacing(20);
    hostLayout->addWidget(lblPort);
    hostLayout->addWidget(spinPort);
    hostLayout->addStretch();
    proxyLayout->addLayout(hostLayout);

    QHBoxLayout *strategyLayout = new QHBoxLayout();
    QLabel *lblStrat = new QLabel("Load Balancing Policy:");
    QComboBox *comboStrat = new QComboBox();
    comboStrat->addItem("Least Connections (Route to lowest load key)");
    comboStrat->addItem("Round Robin (Sequential rotation)");
    comboStrat->addItem("Rate-Limit Priority (Max TPM/RPM capacity first)");
    comboStrat->addItem("Failover (Backup keys activated only on 429 errors)");
    comboStrat->setFixedWidth(360);

    strategyLayout->addWidget(lblStrat);
    strategyLayout->addWidget(comboStrat);
    strategyLayout->addStretch();
    proxyLayout->addLayout(strategyLayout);

    mainLayout->addWidget(grpProxy);

    // 2. Rate Limiting & Cooldown Rules
    QGroupBox *grpRules = new QGroupBox("Rate Limiting & Cooldown Rules");
    QVBoxLayout *rulesLayout = new QVBoxLayout(grpRules);
    rulesLayout->setContentsMargins(10, 14, 10, 10);
    rulesLayout->setSpacing(8);

    QHBoxLayout *coolLayout = new QHBoxLayout();
    QLabel *lblCooldown = new QLabel("429 Cooldown Period (seconds):");
    QSpinBox *spinCool = new QSpinBox();
    spinCool->setRange(5, 3600);
    spinCool->setValue(60);
    spinCool->setFixedWidth(100);

    coolLayout->addWidget(lblCooldown);
    coolLayout->addWidget(spinCool);
    coolLayout->addStretch();
    rulesLayout->addLayout(coolLayout);

    mainLayout->addWidget(grpRules);

    // 3. System & Logging Options
    QGroupBox *grpSystem = new QGroupBox("System & Tray Integration");
    QVBoxLayout *sysLayout = new QVBoxLayout(grpSystem);
    sysLayout->setContentsMargins(10, 14, 10, 10);
    sysLayout->setSpacing(6);

    QCheckBox *chkTray = new QCheckBox("Minimize to system tray on window close");
    chkTray->setChecked(true);

    QCheckBox *chkNotify = new QCheckBox("Show desktop notifications on rate-limit key rotations");
    chkNotify->setChecked(true);

    QCheckBox *chkAutostart = new QCheckBox("Auto-start proxy server when application launches");
    chkAutostart->setChecked(true);

    sysLayout->addWidget(chkTray);
    sysLayout->addWidget(chkNotify);
    sysLayout->addWidget(chkAutostart);

    mainLayout->addWidget(grpSystem);

    // Bottom Action Bar
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnSave = new QPushButton("Save Settings");
    btnSave->setObjectName("PrimaryButton");
    btnSave->setFixedWidth(140);

    btnLayout->addWidget(btnSave);
    btnLayout->addStretch();

    mainLayout->addLayout(btnLayout);
    mainLayout->addStretch();
}
