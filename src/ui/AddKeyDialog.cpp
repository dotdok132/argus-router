#include "AddKeyDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>

AddKeyDialog::AddKeyDialog(KeyPoolManager *poolMgr, QWidget *parent)
    : QDialog(parent), m_poolMgr(poolMgr) {
    setWindowTitle("Add API Key to Pool");
    resize(480, 350);
    setupUi();
}

void AddKeyDialog::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    QLabel *lblHeader = new QLabel("Add API Key (Duplicate Check Active)");
    lblHeader->setStyleSheet("font-size: 14px; font-weight: 700; color: #ffffff;");
    mainLayout->addWidget(lblHeader);

    // Warning Banner for Duplicate Keys
    m_lblWarning = new QLabel("");
    m_lblWarning->setStyleSheet("color: #ce9178; font-weight: 600; font-size: 11.5px; background: rgba(206, 145, 120, 0.1); border: 1px solid rgba(206, 145, 120, 0.3); padding: 5px 8px; border-radius: 4px;");
    m_lblWarning->hide();
    mainLayout->addWidget(m_lblWarning);

    QFormLayout *form = new QFormLayout();
    form->setSpacing(10);

    m_comboProvider = new QComboBox();
    m_comboProvider->addItem("Google Gemini", "gemini");
    m_comboProvider->addItem("Yandex AI Studio", "yandex");
    m_comboProvider->addItem("DeepSeek API", "deepseek");
    m_comboProvider->addItem("Groq Speed Pool", "groq");
    m_comboProvider->addItem("OpenRouter Auto-Free", "openrouter");
    m_comboProvider->addItem("Mistral AI", "mistral");
    m_comboProvider->addItem("Anthropic Claude", "anthropic");
    m_comboProvider->addItem("Together AI", "together");
    m_comboProvider->addItem("Fireworks AI", "fireworks");
    m_comboProvider->addItem("Perplexity AI", "perplexity");
    m_comboProvider->addItem("Cerebras Inference", "cerebras");
    m_comboProvider->addItem("SambaNova Systems", "sambanova");
    m_comboProvider->addItem("Ollama (Local)", "ollama");
    m_comboProvider->addItem("Custom OpenAI-Compatible", "custom");

    m_editAlias = new QLineEdit();
    m_editAlias->setPlaceholderText("Auto-generated or custom alias");

    m_editKey = new QLineEdit();
    m_editKey->setEchoMode(QLineEdit::Password);
    m_editKey->setPlaceholderText("Paste API key here (AIza... / AQVN... / sk-...)");
    connect(m_editKey, &QLineEdit::textChanged, this, &AddKeyDialog::checkDuplicateKey);

    m_spinRpm = new QSpinBox();
    m_spinRpm->setRange(0, 10000);
    m_spinRpm->setSpecialValueText("Auto (Header Discovery)");
    m_spinRpm->setValue(0);

    m_spinTpm = new QSpinBox();
    m_spinTpm->setRange(0, 10000000);
    m_spinTpm->setSpecialValueText("Auto (Header Discovery)");
    m_spinTpm->setValue(0);
    m_spinTpm->setSingleStep(50000);

    m_comboPriority = new QComboBox();
    m_comboPriority->addItem("High");
    m_comboPriority->addItem("Medium");
    m_comboPriority->addItem("Low");
    m_comboPriority->setCurrentText("High");

    auto updateDefaults = [this]() {
        QString p = m_comboProvider->currentData().toString();
        if (p == "gemini") {
            m_editAlias->setText("Gemini Key");
        } else if (p == "yandex") {
            m_editAlias->setText("Yandex AI Studio Key");
        } else if (p == "deepseek") {
            m_editAlias->setText("DeepSeek Key");
        } else if (p == "groq") {
            m_editAlias->setText("Groq Key");
        } else if (p == "openrouter") {
            m_editAlias->setText("OpenRouter Key");
        } else if (p == "mistral") {
            m_editAlias->setText("Mistral Key");
        } else if (p == "anthropic") {
            m_editAlias->setText("Anthropic Key");
        } else if (p == "together") {
            m_editAlias->setText("Together Key");
        } else if (p == "fireworks") {
            m_editAlias->setText("Fireworks Key");
        } else if (p == "perplexity") {
            m_editAlias->setText("Perplexity Key");
        } else if (p == "cerebras") {
            m_editAlias->setText("Cerebras Key");
        } else if (p == "sambanova") {
            m_editAlias->setText("SambaNova Key");
        } else if (p == "ollama") {
            m_editAlias->setText("Ollama Local");
        } else {
            m_editAlias->setText("Custom Key");
        }
        m_spinRpm->setValue(0);
        m_spinTpm->setValue(0);
    };

    connect(m_comboProvider, QOverload<int>::of(&QComboBox::currentIndexChanged), this, updateDefaults);
    updateDefaults();

    form->addRow("Provider:", m_comboProvider);
    form->addRow("Key Alias:", m_editAlias);
    form->addRow("API Key:", m_editKey);
    form->addRow("RPM (Requests/min):", m_spinRpm);
    form->addRow("TPM (Tokens/min):", m_spinTpm);
    form->addRow("Priority:", m_comboPriority);

    mainLayout->addLayout(form);

    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnCancel = new QPushButton("Cancel");
    QPushButton *btnSave = new QPushButton("Save & Add Key");
    btnSave->setObjectName("PrimaryButton");

    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnSave, &QPushButton::clicked, this, [this]() {
        if (m_editKey->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Missing Key", "Please paste or enter a valid API key.");
            return;
        }

        if (m_poolMgr && m_poolMgr->isDuplicateKey(m_editKey->text())) {
            QMessageBox::StandardButton res = QMessageBox::warning(
                this,
                "Duplicate Key Warning",
                "This API key is ALREADY in your pool!\nAre you sure you want to add it again as a duplicate?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
            );
            if (res != QMessageBox::Yes) {
                return;
            }
        }
        accept();
    });

    btnLayout->addStretch();
    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnSave);

    mainLayout->addLayout(btnLayout);
}

void AddKeyDialog::checkDuplicateKey(const QString &text) {
    if (m_poolMgr && m_poolMgr->isDuplicateKey(text)) {
        m_lblWarning->setText("DUPLICATE KEY WARNING: This API key is already in your pool!");
        m_lblWarning->show();
        m_editKey->setStyleSheet("border: 1px solid #ce9178; color: #f14c4c;");
    } else {
        m_lblWarning->hide();
        m_editKey->setStyleSheet("");
    }
}

ApiKeyItem AddKeyDialog::getApiKeyItem() const {
    ApiKeyItem item;
    item.provider = m_comboProvider->currentText();
    item.alias = m_editAlias->text().trimmed().isEmpty() ? QString("%1 Key").arg(item.provider) : m_editAlias->text().trimmed();
    item.key = m_editKey->text().trimmed();
    item.rpmLimit = m_spinRpm->value();
    item.tpmLimit = m_spinTpm->value();
    item.priority = m_comboPriority->currentText();
    item.status = "Untested";
    item.enabled = true;
    return item;
}
