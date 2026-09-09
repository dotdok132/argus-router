#ifndef HTTPPROXYSERVER_H
#define HTTPPROXYSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "KeyPoolManager.h"

class HttpProxyServer : public QObject {
    Q_OBJECT

public:
    explicit HttpProxyServer(KeyPoolManager *poolMgr, QObject *parent = nullptr);
    ~HttpProxyServer();

    bool start(quint16 port = 8080);
    void stop();

    bool isListening() const { return m_tcpServer && m_tcpServer->isListening(); }
    quint16 serverPort() const { return m_tcpServer ? m_tcpServer->serverPort() : 0; }

signals:
    void logTraffic(const QString &time, const QString &clientIp, const QString &endpoint, const QString &provider, const QString &keyAlias, const QString &status, const QString &latency);

private slots:
    void onNewConnection();

private:
    void processSocket(QTcpSocket *socket);
    void sendHttpResponse(QTcpSocket *socket, int statusCode, const QByteArray &content, const QString &contentType = "application/json");

    KeyPoolManager *m_poolMgr;
    QTcpServer *m_tcpServer;
    QNetworkAccessManager *m_netManager;
    int m_rrIndex = 0;
};

#endif // HTTPPROXYSERVER_H
