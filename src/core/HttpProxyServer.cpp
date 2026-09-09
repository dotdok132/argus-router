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
        addM("auto", "argus");
        addM("gemini-3.6-flash", "google");
        addM("yandexgpt/latest", "yandex");
        addM("yandexgpt-lite/latest", "yandex");
        addM("deepseek-chat", "deepseek");
        addM("deepseek-reasoner", "deepseek");
        addM("llama-3.3-70b-versatile", "groq");
        addM("meta-llama/llama-3.3-70b-instruct", "openrouter");
        addM("mistral-small-latest", "mistral");
        addM("claude-3-5-sonnet-20241022", "anthropic");
        addM("meta-llama/Llama-3.3-70B-Instruct-Turbo", "together");
        addM("accounts/fireworks/models/llama-v3p3-70b-instruct", "fireworks");
        addM("sonar-pro", "perplexity");
        addM("llama3.1-70b", "cerebras");
        addM("Meta-Llama-3.3-70B-Instruct", "sambanova");
        addM("qwen2.5:0.5b", "ollama");
        addM("gpt-4o", "openai");
        addM("gpt-4o-mini", "openai");

        resObj["data"] = modelsArray;
        sendHttpResponse(socket, 200, QJsonDocument(resObj).toJson(QJsonDocument::Compact));
        return;
    }

    // Handle POST /v1/chat/completions
    if (method == "POST" && (path.contains("/chat/completions") || path.contains("/completions") || path == "/")) {
        forwardChatCompletion(socket, bodyData, clientIp, path, 0);
        return;
    }

    // Default Fallback Response
    sendHttpResponse(socket, 200, "{\"status\": \"Argus Token Router active\", \"endpoint\": \"/v1/chat/completions\"}");
}

void HttpProxyServer::forwardChatCompletion(QTcpSocket *socket, const QByteArray &bodyData, const QString &clientIp, const QString &path, int keyAttemptIndex, int toolDepth) {
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

    if (activeKeys.isEmpty() || keyAttemptIndex >= activeKeys.size()) {
        QJsonObject errObj;
        QJsonObject errInner;
        errInner["message"] = "All API keys in Argus Token Router pool failed or rate limited!";
        errInner["type"] = "all_keys_failed";
        errObj["error"] = errInner;
        sendHttpResponse(socket, 503, QJsonDocument(errObj).toJson(QJsonDocument::Compact));
        emit logTraffic(QDateTime::currentDateTime().toString("HH:mm:ss"), clientIp, path, "None", "None", "503 All Failed", "0ms");
        return;
    }

    // Calculate target key index based on selected Queue Strategy
    QueueStrategy strat = m_poolMgr->getQueueStrategy();
    int targetIdx = 0;
    if (strat == QueueStrategy::SequentialPriority) {
        targetIdx = keyAttemptIndex % activeKeys.size();
    } else if (strat == QueueStrategy::RoundRobin) {
        targetIdx = (m_rrIndex + keyAttemptIndex) % activeKeys.size();
    } else { // LeastLoaded
        targetIdx = keyAttemptIndex % activeKeys.size();
    }

    ApiKeyItem selectedKey = activeKeys[targetIdx];

    // Prepare Upstream Request
    QUrl upstreamUrl;
    QString pLower = selectedKey.provider.toLower();

    if (pLower.contains("gemini")) {
        upstreamUrl = QUrl("https://generativelanguage.googleapis.com/v1beta/openai/chat/completions");
    } else if (pLower.contains("yandex")) {
        upstreamUrl = QUrl("https://llm.api.cloud.yandex.net/foundationModels/v1/chat/completions");
    } else if (pLower.contains("deepseek")) {
        upstreamUrl = QUrl("https://api.deepseek.com/chat/completions");
    } else if (pLower.contains("groq")) {
        upstreamUrl = QUrl("https://api.groq.com/openai/v1/chat/completions");
    } else if (pLower.contains("openrouter")) {
        upstreamUrl = QUrl("https://openrouter.ai/api/v1/chat/completions");
    } else if (pLower.contains("mistral")) {
        upstreamUrl = QUrl("https://api.mistral.ai/v1/chat/completions");
    } else if (pLower.contains("anthropic")) {
        upstreamUrl = QUrl("https://api.anthropic.com/v1/messages");
    } else if (pLower.contains("together")) {
        upstreamUrl = QUrl("https://api.together.xyz/v1/chat/completions");
    } else if (pLower.contains("fireworks")) {
        upstreamUrl = QUrl("https://api.fireworks.ai/inference/v1/chat/completions");
    } else if (pLower.contains("perplexity")) {
        upstreamUrl = QUrl("https://api.perplexity.ai/chat/completions");
    } else if (pLower.contains("cerebras")) {
        upstreamUrl = QUrl("https://api.cerebras.ai/v1/chat/completions");
    } else if (pLower.contains("sambanova")) {
        upstreamUrl = QUrl("https://api.sambanova.ai/v1/chat/completions");
    } else if (pLower.contains("ollama")) {
        upstreamUrl = QUrl("http://localhost:11434/v1/chat/completions");
    } else {
        upstreamUrl = QUrl("https://api.openai.com/v1/chat/completions");
    }

    // Auto Model Alias Rewriter & Memory Tools Injection
    QByteArray payloadToSend = bodyData;
    QString targetModel;
    if (pLower.contains("gemini")) {
        targetModel = m_poolMgr->getBestGeminiModel(selectedKey.key);
    } else if (pLower.contains("yandex")) {
        targetModel = "yandexgpt/latest";
    } else if (pLower.contains("deepseek")) {
        targetModel = "deepseek-chat";
    } else if (pLower.contains("groq")) {
        targetModel = "llama-3.3-70b-versatile";
    } else if (pLower.contains("openrouter")) {
        targetModel = "meta-llama/llama-3.3-70b-instruct";
    } else if (pLower.contains("mistral")) {
        targetModel = "mistral-small-latest";
    } else if (pLower.contains("anthropic")) {
        targetModel = "claude-3-5-sonnet-20241022";
    } else if (pLower.contains("together")) {
        targetModel = "meta-llama/Llama-3.3-70B-Instruct-Turbo";
    } else if (pLower.contains("fireworks")) {
        targetModel = "accounts/fireworks/models/llama-v3p3-70b-instruct";
    } else if (pLower.contains("perplexity")) {
        targetModel = "sonar-pro";
    } else if (pLower.contains("cerebras")) {
        targetModel = "llama3.1-70b";
    } else if (pLower.contains("sambanova")) {
        targetModel = "Meta-Llama-3.3-70B-Instruct";
    } else if (pLower.contains("ollama")) {
        targetModel = "qwen2.5:0.5b";
    } else {
        targetModel = "gpt-4o-mini";
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(bodyData);
    QJsonObject jsonObj = jsonDoc.isObject() ? jsonDoc.object() : QJsonObject();

    if (!jsonObj.isEmpty()) {
        QString reqModel = jsonObj["model"].toString().trimmed();

        if (reqModel.isEmpty() || reqModel == "default" || reqModel == "custom/default" || reqModel == "auto") {
            jsonObj["model"] = targetModel;
        } else {
            targetModel = reqModel;
        }

        // Inject Memory Tool Schemas & System Instruction on initial request (toolDepth == 0)
        if (m_memMgr != nullptr && toolDepth == 0) {
            if (!jsonObj.contains("tools")) {
                QJsonArray toolsArray;

                QJsonObject getMemTool;
                getMemTool["type"] = "function";
                QJsonObject getMemFn;
                getMemFn["name"] = "get_memory";
                getMemFn["description"] = "Read the contents of a specific memory .md file from the local library.";
                QJsonObject getMemParams;
                getMemParams["type"] = "object";
                QJsonObject getMemProps;
                QJsonObject nameProp;
                nameProp["type"] = "string";
                nameProp["description"] = "The file name of the memory module, e.g. 'code_requirements.md' or 'user_character.md'.";
                getMemProps["name"] = nameProp;
                getMemParams["properties"] = getMemProps;
                QJsonArray reqArray;
                reqArray.append("name");
                getMemParams["required"] = reqArray;
                getMemFn["parameters"] = getMemParams;
                getMemTool["function"] = getMemFn;
                toolsArray.append(getMemTool);

                QJsonObject listMemTool;
                listMemTool["type"] = "function";
                QJsonObject listMemFn;
                listMemFn["name"] = "list_memories";
                listMemFn["description"] = "List all available memory .md files in the local memory library.";
                listMemTool["function"] = listMemFn;
                toolsArray.append(listMemTool);

                jsonObj["tools"] = toolsArray;
            }

            QJsonArray messages = jsonObj["messages"].toArray();
            QString memInstruction = "You have access to a local memory library via tools ('get_memory', 'list_memories'). When the user asks about user identity, preferences, code requirements, or project context, automatically call get_memory or list_memories to inspect memory files before answering.";

            bool sysFound = false;
            if (!messages.isEmpty()) {
                QJsonObject firstMsg = messages[0].toObject();
                if (firstMsg["role"].toString() == "system") {
                    QString existingContent = firstMsg["content"].toString();
                    firstMsg["content"] = existingContent + "\n\n" + memInstruction;
                    messages[0] = firstMsg;
                    sysFound = true;
                }
            }
            if (!sysFound) {
                QJsonObject sysMsg;
                sysMsg["role"] = "system";
                sysMsg["content"] = memInstruction;
                messages.prepend(sysMsg);
            }
            jsonObj["messages"] = messages;
        }

        payloadToSend = QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
    }

    QString logEndpoint = QString("%1 (%2)").arg(path, targetModel);

    QNetworkRequest upRequest(upstreamUrl);
    upRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (pLower.contains("yandex")) {
        upRequest.setRawHeader("Authorization", QString("Api-Key %1").arg(selectedKey.key).toUtf8());
    } else if (pLower.contains("anthropic")) {
        upRequest.setRawHeader("x-api-key", selectedKey.key.toUtf8());
        upRequest.setRawHeader("anthropic-version", "2023-06-01");
    } else {
        upRequest.setRawHeader("Authorization", QString("Bearer %1").arg(selectedKey.key).toUtf8());
    }

    QElapsedTimer *timer = new QElapsedTimer();
    timer->start();

    QNetworkReply *reply = m_netManager->post(upRequest, payloadToSend);

    connect(reply, &QNetworkReply::finished, this, [this, socket, reply, timer, selectedKey, clientIp, logEndpoint, bodyData, jsonObj, keyAttemptIndex, activeKeys, toolDepth, path]() {
        reply->deleteLater();
        qint64 latencyMs = timer->elapsed();
        delete timer;

        if (m_poolMgr) {
            m_poolMgr->updateKeyRateLimitFromHeaders(selectedKey.id, reply->rawHeaderPairs());
        }

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 0) {
            statusCode = (reply->error() == QNetworkReply::NoError) ? 200 : 502;
        }

        QByteArray replyData = reply->readAll();
        qint64 totalTokens = 0;

        QJsonDocument resDoc = QJsonDocument::fromJson(replyData);
        if (resDoc.isObject()) {
            QJsonObject resObj = resDoc.object();
            if (resObj.contains("usage") && resObj["usage"].isObject()) {
                QJsonObject usage = resObj["usage"].toObject();
                totalTokens = usage["total_tokens"].toVariant().toLongLong();
            }

            // Check if model requested a tool execution (Memory Lookup)
            if (statusCode == 200 && m_memMgr != nullptr && toolDepth < 3) {
                QJsonArray choices = resObj["choices"].toArray();
                if (!choices.isEmpty()) {
                    QJsonObject firstChoice = choices[0].toObject();
                    QJsonObject messageObj = firstChoice["message"].toObject();

                    if (messageObj.contains("tool_calls") && messageObj["tool_calls"].isArray()) {
                        QJsonArray toolCalls = messageObj["tool_calls"].toArray();
                        if (!toolCalls.isEmpty()) {
                            qDebug() << "[Argus Memory] Intercepted tool call from LLM - depth" << toolDepth;

                            QJsonObject updatedJson = jsonObj;
                            QJsonArray messages = updatedJson["messages"].toArray();
                            messages.append(messageObj);

                            for (const auto &tcVal : toolCalls) {
                                QJsonObject tc = tcVal.toObject();
                                QString callId = tc["id"].toString();
                                QJsonObject fnObj = tc["function"].toObject();
                                QString fnName = fnObj["name"].toString();
                                QString fnArgs = fnObj["arguments"].toString();

                                QString toolResultStr;

                                if (fnName == "get_memory") {
                                    QJsonDocument argDoc = QJsonDocument::fromJson(fnArgs.toUtf8());
                                    QString memName;
                                    if (argDoc.isObject()) {
                                        memName = argDoc.object()["name"].toString();
                                    } else {
                                        memName = fnArgs;
                                    }

                                    QString content = m_memMgr->readMemory(memName);
                                    if (content.isEmpty()) {
                                        toolResultStr = QString("Memory module '%1' not found.").arg(memName);
                                    } else {
                                        toolResultStr = QString("Memory Module (.memory/%1):\n\n%2").arg(memName, content);
                                    }
                                    qDebug() << "[Argus Memory] Executed get_memory(" << memName << ")";
                                } else if (fnName == "list_memories") {
                                    QStringList files = m_memMgr->listMemories();
                                    toolResultStr = QString("Available Memory Modules: %1").arg(files.join(", "));
                                    qDebug() << "[Argus Memory] Executed list_memories()";
                                } else {
                                    toolResultStr = "Unknown tool.";
                                }

                                QJsonObject toolRespMsg;
                                toolRespMsg["role"] = "tool";
                                toolRespMsg["tool_call_id"] = callId;
                                toolRespMsg["content"] = toolResultStr;
                                messages.append(toolRespMsg);
                            }

                            updatedJson["messages"] = messages;
                            QByteArray updatedPayload = QJsonDocument(updatedJson).toJson(QJsonDocument::Compact);

                            // Transparent Internal Roundtrip Loop!
                            forwardChatCompletion(socket, updatedPayload, clientIp, path, keyAttemptIndex, toolDepth + 1);
                            return;
                        }
                    }
                }
            }
        }

        if (totalTokens == 0 && statusCode == 200 && !replyData.isEmpty()) {
            totalTokens = qMax<qint64>(1, (bodyData.size() + replyData.size()) / 4);
        }

        if (m_poolMgr && totalTokens > 0 && statusCode == 200) {
            m_poolMgr->recordTokenUsage(selectedKey.id, totalTokens);
        }

        // If upstream error (429 Rate Limit, 503 Service Unavailable, 502 Bad Gateway, 401 Invalid Key, 500 Server Error):
        // Automatically failover to next key in pool!
        if (statusCode == 429 || statusCode == 503 || statusCode == 502 || statusCode == 401 || statusCode == 500) {
            qWarning() << "[Argus Failover] Key" << selectedKey.alias << "(" << selectedKey.provider << ") failed with status" << statusCode << "- Failing over to next key!";
            
            emit logTraffic(
                QDateTime::currentDateTime().toString("HH:mm:ss"),
                clientIp,
                logEndpoint,
                selectedKey.provider,
                selectedKey.alias,
                QString("%1 (Failover)").arg(statusCode),
                QString("%1ms").arg(latencyMs),
                0
            );

            // Retry seamlessly with next key attempt
            forwardChatCompletion(socket, bodyData, clientIp, logEndpoint, keyAttemptIndex + 1, toolDepth);
            return;
        }

        // Success: advance Round-Robin index for next client request
        m_rrIndex = (m_rrIndex + 1) % activeKeys.size();

        sendHttpResponse(socket, statusCode, replyData, "application/json");

        emit logTraffic(
            QDateTime::currentDateTime().toString("HH:mm:ss"),
            clientIp,
            logEndpoint,
            selectedKey.provider,
            selectedKey.alias,
            QString("%1 OK").arg(statusCode),
            QString("%1ms").arg(latencyMs),
            totalTokens
        );
    });
}
