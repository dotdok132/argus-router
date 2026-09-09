#include "HttpProxyServer.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QDebug>

HttpProxyServer::HttpProxyServer(KeyPoolManager *poolMgr, QObject *parent)
    : QObject(parent), m_poolMgr(poolMgr) {
    m_tcpServer = new QTcpServer(this);
    m_netManager = new QNetworkAccessManager(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &HttpProxyServer::onNewConnection);
}

HttpProxyServer::~HttpProxyServer() {
    stop();
}

bool HttpProxyServer::start(quint16 port) {
    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
    }
    bool ok = m_tcpServer->listen(QHostAddress::Any, port);
    if (ok) {
        qDebug() << "[Argus Proxy] Listening on http://127.0.0.1:" << port;
    } else {
        qWarning() << "[Argus Proxy] Failed to listen on port" << port << ":" << m_tcpServer->errorString();
    }
    return ok;
}

void HttpProxyServer::stop() {
    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
    }
}

void HttpProxyServer::onNewConnection() {
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket *socket = m_tcpServer->nextPendingConnection();
        if (!socket) continue;

        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            processSocket(socket);
        });

        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    }
}

void HttpProxyServer::sendHttpResponse(QTcpSocket *socket, int statusCode, const QByteArray &content, const QString &contentType) {
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

    QString statusText = (statusCode == 200) ? "OK" : (statusCode == 429 ? "Too Many Requests" : (statusCode == 401 ? "Unauthorized" : "Internal Server Error"));
    QByteArray response = QString("HTTP/1.1 %1 %2\r\n"
                                  "Content-Type: %3\r\n"
                                  "Content-Length: %4\r\n"
                                  "Access-Control-Allow-Origin: *\r\n"
                                  "Access-Control-Allow-Headers: *\r\n"
                                  "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                                  "Connection: close\r\n\r\n")
                              .arg(statusCode)
                              .arg(statusText)
                              .arg(contentType)
                              .arg(content.size())
                              .toUtf8() + content;

    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();
}

void HttpProxyServer::processSocket(QTcpSocket *socket) {
    if (!socket || socket->bytesAvailable() == 0) return;

    QByteArray requestData = socket->readAll();
    int headerEnd = requestData.indexOf("\r\n\r\n");
    if (headerEnd == -1) return;

    QByteArray headerData = requestData.left(headerEnd);
    QByteArray bodyData = requestData.mid(headerEnd + 4);

    QList<QByteArray> lines = headerData.split('\n');
    if (lines.isEmpty()) return;

    QList<QByteArray> requestLine = lines[0].trimmed().split(' ');
    if (requestLine.size() < 2) return;

    QString method = QString::fromUtf8(requestLine[0]).toUpper();
    QString path = QString::fromUtf8(requestLine[1]);

    QString clientIp = socket->peerAddress().toString();
    if (clientIp.startsWith("::ffff:")) clientIp = clientIp.mid(7);
    if (clientIp.isEmpty() || clientIp == "::1") clientIp = "127.0.0.1";

    // Handle OPTIONS CORS preflight
    if (method == "OPTIONS") {
        sendHttpResponse(socket, 200, "", "text/plain");
        return;
    }

    // Handle GET /v1/models
    if (method == "GET" && path.contains("/models")) {
        QJsonObject resObj;
        resObj["object"] = "list";
        QJsonArray modelsArray;

        auto addM = [&modelsArray](const QString &id, const QString &owner) {
            QJsonObject m;
            m["id"] = id;
            m["object"] = "model";
            m["owned_by"] = owner;
            modelsArray.append(m);
        };

        addM("default", "argus");
        addM("custom/default", "argus");
        addM("gemini-2.0-flash", "google");
        addM("llama-3.3-70b-versatile", "groq");
        addM("google/gemini-2.0-flash-001", "openrouter");
        addM("auto", "argus");
        addM("gpt-4o", "openai");
        addM("gpt-4o-mini", "openai");

        resObj["data"] = modelsArray;
        sendHttpResponse(socket, 200, QJsonDocument(resObj).toJson(QJsonDocument::Compact));
        return;
    }

    // Handle POST /v1/chat/completions
    if (method == "POST" && (path.contains("/chat/completions") || path.contains("/completions") || path == "/")) {
        if (!m_poolMgr) {
            sendHttpResponse(socket, 500, "{\"error\": \"Key pool manager null\"}");
            return;
        }

        const auto &keys = m_poolMgr->getKeys();
        QList<ApiKeyItem> activeKeys;
        for (const auto &k : keys) {
            if (k.enabled && !k.status.contains("Invalid")) {
                activeKeys.append(k);
            }
        }

        if (activeKeys.isEmpty()) {
            QJsonObject errObj;
            QJsonObject errInner;
            errInner["message"] = "No active API keys available in Argus Token Router pool!";
            errInner["type"] = "no_active_keys";
            errObj["error"] = errInner;
            sendHttpResponse(socket, 503, QJsonDocument(errObj).toJson(QJsonDocument::Compact));
            emit logTraffic(QDateTime::currentDateTime().toString("HH:mm:ss"), clientIp, path, "None", "None", "503 No Keys", "0ms");
            return;
        }

        // Select key via Round-Robin
        m_rrIndex = (m_rrIndex + 1) % activeKeys.size();
        ApiKeyItem selectedKey = activeKeys[m_rrIndex];

        // Prepare Upstream Request
        QUrl upstreamUrl;
        QString pLower = selectedKey.provider.toLower();

        if (pLower.contains("gemini")) {
            upstreamUrl = QUrl("https://generativelanguage.googleapis.com/v1beta/openai/chat/completions");
        } else if (pLower.contains("groq")) {
            upstreamUrl = QUrl("https://api.groq.com/openai/v1/chat/completions");
        } else if (pLower.contains("openrouter")) {
            upstreamUrl = QUrl("https://openrouter.ai/api/v1/chat/completions");
        } else if (pLower.contains("anthropic")) {
            upstreamUrl = QUrl("https://api.anthropic.com/v1/messages");
        } else {
            upstreamUrl = QUrl("https://api.openai.com/v1/chat/completions");
        }

        // Auto Model Alias Rewriter: Map generic "default" / "custom/default" / unknown model names to valid provider models
        QByteArray payloadToSend = bodyData;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(bodyData);
        if (jsonDoc.isObject()) {
            QJsonObject jsonObj = jsonDoc.object();
            QString reqModel = jsonObj["model"].toString().trimmed();

            QString targetModel;
            if (pLower.contains("gemini")) {
                targetModel = m_poolMgr ? m_poolMgr->getBestGeminiModel(selectedKey.key) : "gemini-3.6-flash";
            } else if (pLower.contains("groq")) {
                targetModel = "llama-3.3-70b-versatile";
            } else if (pLower.contains("openrouter")) {
                targetModel = "meta-llama/llama-3.3-70b-instruct";
            } else if (pLower.contains("anthropic")) {
                targetModel = "claude-3-5-sonnet-20241022";
            } else {
                targetModel = "gpt-4o-mini";
            }

            if (reqModel.isEmpty() || reqModel == "default" || reqModel == "custom/default" || reqModel == "auto" ||
                (pLower.contains("gemini") && !reqModel.contains("gemini")) ||
                (pLower.contains("groq") && !reqModel.contains("llama") && !reqModel.contains("mixtral") && !reqModel.contains("deepseek")) ||
                (pLower.contains("openrouter") && !reqModel.contains("/"))) {
                jsonObj["model"] = targetModel;
                payloadToSend = QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
            }
        }

        QNetworkRequest upRequest(upstreamUrl);
        upRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        if (pLower.contains("gemini")) {
            upRequest.setRawHeader("Authorization", QString("Bearer %1").arg(selectedKey.key).toUtf8());
        } else if (pLower.contains("groq") || pLower.contains("openrouter")) {
            upRequest.setRawHeader("Authorization", QString("Bearer %1").arg(selectedKey.key).toUtf8());
        } else if (pLower.contains("anthropic")) {
            upRequest.setRawHeader("x-api-key", selectedKey.key.toUtf8());
            upRequest.setRawHeader("anthropic-version", "2023-06-01");
        } else {
            upRequest.setRawHeader("Authorization", QString("Bearer %1").arg(selectedKey.key).toUtf8());
        }

        QElapsedTimer *timer = new QElapsedTimer();
        timer->start();

        QNetworkReply *reply = m_netManager->post(upRequest, payloadToSend);

        connect(reply, &QNetworkReply::finished, this, [this, socket, reply, timer, selectedKey, clientIp, path]() {
            reply->deleteLater();
            qint64 latencyMs = timer->elapsed();
            delete timer;

            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (statusCode == 0) {
                statusCode = (reply->error() == QNetworkReply::NoError) ? 200 : 502;
            }

            QByteArray replyData = reply->readAll();
            sendHttpResponse(socket, statusCode, replyData, "application/json");

            QString statusStr = QString("%1 OK").arg(statusCode);
            if (statusCode >= 400) {
                statusStr = QString("%1 Error").arg(statusCode);
            }

            emit logTraffic(
                QDateTime::currentDateTime().toString("HH:mm:ss"),
                clientIp,
                path,
                selectedKey.provider,
                selectedKey.alias,
                statusStr,
                QString("%1ms").arg(latencyMs)
            );
        });
        return;
    }

    // Default Fallback Response
    sendHttpResponse(socket, 200, "{\"status\": \"Argus Token Router active\", \"endpoint\": \"/v1/chat/completions\"}");
}
