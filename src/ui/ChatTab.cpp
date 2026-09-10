#include "ChatTab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QEvent>
#include <QKeyEvent>
#include <QDateTime>
#include <QScrollBar>

ChatTab::ChatTab(KeyPoolManager *poolMgr, QWidget *parent)
    : QWidget(parent), m_poolMgr(poolMgr), m_isGenerating(false) {

    m_netMgr = new QNetworkAccessManager(this);
    connect(m_netMgr, &QNetworkAccessManager::finished, this, &ChatTab::onRequestFinished);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    // Top Controls Toolbar
    QWidget *topBar = new QWidget(this);
    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *lblModel = new QLabel("Model:", this);
    lblModel->setStyleSheet("color: #a0a6b8; font-weight: bold; font-size: 12px;");

    m_cboModel = new QComboBox(this);
    m_cboModel->addItem("auto (Smart Memory & Key Routing)", "auto");
    m_cboModel->addItem("gemini-3.6-flash (Google)", "gemini-3.6-flash");
    m_cboModel->addItem("deepseek-chat (DeepSeek)", "deepseek-chat");
    m_cboModel->addItem("llama-3.3-70b-versatile (Groq)", "llama-3.3-70b-versatile");
    m_cboModel->addItem("claude-3-5-sonnet-20241022 (Anthropic)", "claude-3-5-sonnet-20241022");
    m_cboModel->addItem("mistral-small-latest (Mistral)", "mistral-small-latest");
    m_cboModel->addItem("meta-llama/llama-3.3-70b-instruct (OpenRouter)", "meta-llama/llama-3.3-70b-instruct");
    m_cboModel->setStyleSheet("QComboBox { background-color: #1e222b; color: #d0d7de; border: 1px solid #30364d; border-radius: 4px; padding: 4px 8px; font-size: 12px; }");

    m_btnClear = new QPushButton("Clear History", this);
    m_btnClear->setStyleSheet("QPushButton { background-color: #2b303d; color: #a0a6b8; border: 1px solid #3a4052; border-radius: 4px; padding: 5px 12px; font-size: 12px; } QPushButton:hover { background-color: #383f52; color: #ffffff; }");
    connect(m_btnClear, &QPushButton::clicked, this, &ChatTab::onClearHistory);

    m_lblStatus = new QLabel("Ready", this);
    m_lblStatus->setStyleSheet("color: #7d8590; font-size: 11px; margin-left: 10px;");

    topLayout->addWidget(lblModel);
    topLayout->addWidget(m_cboModel);
    topLayout->addWidget(m_btnClear);
    topLayout->addStretch();
    topLayout->addWidget(m_lblStatus);

    mainLayout->addWidget(topBar);

    // Chat Conversation Display
    m_chatDisplay = new QTextBrowser(this);
    m_chatDisplay->setOpenExternalLinks(true);
    m_chatDisplay->setStyleSheet(
        "QTextBrowser { "
        "  background-color: #0f1117; "
        "  color: #e6edf3; "
        "  border: 1px solid #212636; "
        "  border-radius: 6px; "
        "  padding: 12px; "
        "  font-family: 'Segoe UI', Arial, sans-serif; "
        "  font-size: 13px; "
        "} "
    );
    mainLayout->addWidget(m_chatDisplay, 1);

    // Bottom Input Box & Send Button
    QWidget *inputBar = new QWidget(this);
    QHBoxLayout *inputLayout = new QHBoxLayout(inputBar);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(8);

    m_inputEdit = new QTextEdit(this);
    m_inputEdit->setPlaceholderText("Type a message... (Press Enter to send, Shift+Enter for newline)");
    m_inputEdit->setFixedHeight(65);
    m_inputEdit->setStyleSheet(
        "QTextEdit { "
        "  background-color: #161922; "
        "  color: #e6edf3; "
        "  border: 1px solid #30364d; "
        "  border-radius: 6px; "
        "  padding: 8px; "
        "  font-size: 13px; "
        "} "
        "QTextEdit:focus { "
        "  border: 1px solid #007acc; "
        "} "
    );
    m_inputEdit->installEventFilter(this);

    m_btnSend = new QPushButton("Send", this);
    m_btnSend->setFixedSize(85, 65);
    m_btnSend->setStyleSheet(
        "QPushButton { "
        "  background-color: #007acc; "
        "  color: #ffffff; "
        "  font-weight: bold; "
        "  font-size: 13px; "
        "  border: none; "
        "  border-radius: 6px; "
        "} "
        "QPushButton:hover { "
        "  background-color: #0098ff; "
        "} "
        "QPushButton:disabled { "
        "  background-color: #2c3345; "
        "  color: #60687a; "
        "} "
    );
    connect(m_btnSend, &QPushButton::clicked, this, &ChatTab::onSendMessage);

    inputLayout->addWidget(m_inputEdit, 1);
    inputLayout->addWidget(m_btnSend);

    mainLayout->addWidget(inputBar);

    appendSystemNotice("Argus AI Chat initialized. Connected to local proxy at http://127.0.0.1:8080.");
}

bool ChatTab::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_inputEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if ((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)) {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                return false; // Insert newline
            } else {
                onSendMessage();
                return true; // Handle event
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ChatTab::onSendMessage() {
    if (m_isGenerating) return;

    QString text = m_inputEdit->toPlainText().trimmed();
    if (text.isEmpty()) return;

    m_inputEdit->clear();
    appendUserMessage(text);

    // Build message JSON object
    QJsonObject userMsgObj;
    userMsgObj["role"] = "user";
    userMsgObj["content"] = text;
    m_conversationHistory.append(userMsgObj);

    QString selectedModel = m_cboModel->currentData().toString();

    QJsonObject reqObj;
    reqObj["model"] = selectedModel;
    reqObj["messages"] = m_conversationHistory;

    QNetworkRequest request(QUrl("http://127.0.0.1:8080/v1/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray payload = QJsonDocument(reqObj).toJson(QJsonDocument::Compact);

    m_isGenerating = true;
    m_btnSend->setDisabled(true);
    m_lblStatus->setText("Generating response...");
    m_lblStatus->setStyleSheet("color: #e0c060; font-weight: bold; font-size: 11px; margin-left: 10px;");

    m_netMgr->post(request, payload);
}

void ChatTab::onRequestFinished(QNetworkReply *reply) {
    reply->deleteLater();
    m_isGenerating = false;
    m_btnSend->setEnabled(true);

    if (reply->error() != QNetworkReply::NoError) {
        QString errStr = reply->errorString();
        QByteArray errData = reply->readAll();
        if (!errData.isEmpty()) {
            QJsonDocument errDoc = QJsonDocument::fromJson(errData);
            if (errDoc.isObject() && errDoc.object().contains("error")) {
                QJsonObject errObj = errDoc.object()["error"].toObject();
                if (errObj.contains("message")) {
                    errStr = errObj["message"].toString();
                }
            }
        }
        appendSystemNotice(QString("Error: %1").arg(errStr));
        m_lblStatus->setText("Error");
        m_lblStatus->setStyleSheet("color: #f14c4c; font-weight: bold; font-size: 11px; margin-left: 10px;");
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument resDoc = QJsonDocument::fromJson(responseData);

    if (!resDoc.isObject()) {
        appendSystemNotice("Error: Invalid JSON response received from proxy.");
        m_lblStatus->setText("Invalid Response");
        return;
    }

    QJsonObject resObj = resDoc.object();
    QString modelUsed = resObj.contains("model") ? resObj["model"].toString() : m_cboModel->currentData().toString();
    QJsonArray choices = resObj["choices"].toArray();

    if (choices.isEmpty()) {
        appendSystemNotice("Error: Empty choices array in proxy response.");
        m_lblStatus->setText("No Choices");
        return;
    }

    QJsonObject firstChoice = choices[0].toObject();
    QJsonObject messageObj = firstChoice["message"].toObject();

    QString contentStr = messageObj["content"].toString();
    if (contentStr.isEmpty()) {
        contentStr = "(No text response generated)";
    }

    // Save assistant message to conversation history
    QJsonObject assistantMsgObj;
    assistantMsgObj["role"] = "assistant";
    assistantMsgObj["content"] = contentStr;
    m_conversationHistory.append(assistantMsgObj);

    appendAssistantMessage(contentStr, modelUsed);

    m_lblStatus->setText("Ready");
    m_lblStatus->setStyleSheet("color: #7d8590; font-size: 11px; margin-left: 10px;");
}

void ChatTab::onClearHistory() {
    m_conversationHistory = QJsonArray();
    m_chatDisplay->clear();
    appendSystemNotice("Conversation history cleared.");
}

void ChatTab::appendUserMessage(const QString &text) {
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm");
    QString html = QString(
        "<div style='margin-bottom: 12px; text-align: right;'>"
        "  <span style='color: #7d8590; font-size: 10px;'>You • %1</span><br/>"
        "  <div style='display: inline-block; background-color: #1f2736; color: #f0f6fc; border: 1px solid #2d384e; border-radius: 8px; padding: 8px 12px; text-align: left; max-width: 80%%; margin-top: 4px; white-space: pre-wrap;'>"
        "    %2"
        "  </div>"
        "</div>"
    ).arg(timeStr, text.toHtmlEscaped());

    m_chatDisplay->append(html);
    m_chatDisplay->verticalScrollBar()->setValue(m_chatDisplay->verticalScrollBar()->maximum());
}

void ChatTab::appendAssistantMessage(const QString &text, const QString &modelUsed) {
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm");
    QString formattedContent = text.toHtmlEscaped();
    formattedContent.replace("\n", "<br/>");

    QString html = QString(
        "<div style='margin-bottom: 12px; text-align: left;'>"
        "  <span style='color: #58a6ff; font-weight: bold; font-size: 11px;'>Argus AI</span> "
        "  <span style='background-color: #1c2d42; color: #79c0ff; border: 1px solid #264366; border-radius: 3px; padding: 1px 5px; font-size: 9px;'>%1</span> "
        "  <span style='color: #7d8590; font-size: 10px;'>• %2</span><br/>"
        "  <div style='background-color: #161b22; color: #e6edf3; border: 1px solid #30364d; border-radius: 8px; padding: 10px 14px; margin-top: 4px; white-space: pre-wrap; line-height: 1.4;'>"
        "    %3"
        "  </div>"
        "</div>"
    ).arg(modelUsed, timeStr, formattedContent);

    m_chatDisplay->append(html);
    m_chatDisplay->verticalScrollBar()->setValue(m_chatDisplay->verticalScrollBar()->maximum());
}

void ChatTab::appendSystemNotice(const QString &notice) {
    QString html = QString(
        "<div style='margin-bottom: 8px; text-align: center;'>"
        "  <span style='background-color: #21262d; color: #8b949e; border: 1px solid #30363d; border-radius: 12px; padding: 3px 10px; font-size: 10px;'>"
        "    %1"
        "  </span>"
        "</div>"
    ).arg(notice.toHtmlEscaped());

    m_chatDisplay->append(html);
    m_chatDisplay->verticalScrollBar()->setValue(m_chatDisplay->verticalScrollBar()->maximum());
}
