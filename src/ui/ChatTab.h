#ifndef CHATTAB_H
#define CHATTAB_H

#include <QWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "../core/KeyPoolManager.h"

class ChatTab : public QWidget {
    Q_OBJECT

public:
    explicit ChatTab(KeyPoolManager *poolMgr, QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onSendMessage();
    void onClearHistory();
    void onRequestFinished(QNetworkReply *reply);

private:
    void appendUserMessage(const QString &text);
    void appendAssistantMessage(const QString &text, const QString &modelUsed);
    void appendSystemNotice(const QString &notice);

    KeyPoolManager *m_poolMgr;
    QNetworkAccessManager *m_netMgr;

    QTextBrowser *m_chatDisplay;
    QTextEdit *m_inputEdit;
    QComboBox *m_cboModel;
    QPushButton *m_btnSend;
    QPushButton *m_btnClear;
    QLabel *m_lblStatus;

    QJsonArray m_conversationHistory;
    bool m_isGenerating;
};

#endif // CHATTAB_H
