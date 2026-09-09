#include "KeyPoolManager.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QUuid>
#include <QNetworkRequest>
#include <QUrl>
#include <QDateTime>
#include <QTimer>
#include <QDebug>

KeyPoolManager::KeyPoolManager(QObject *parent) : QObject(parent) {
    m_netManager = new QNetworkAccessManager(this);
    loadFromDisk();
}

QString KeyPoolManager::getDefaultConfigPath() const {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.filePath("keys.json");
}

bool KeyPoolManager::isDuplicateKey(const QString &keyStr, const QString &ignoreId) const {
    QString trimmed = keyStr.trimmed();
    if (trimmed.isEmpty()) return false;

    for (const auto &k : m_keys) {
        if (k.id != ignoreId && k.key.trimmed() == trimmed) {
            return true;
        }
    }
    return false;
}

void KeyPoolManager::updateDuplicates() {
    QHash<QString, int> keyCounts;
    for (const auto &k : m_keys) {
        QString keyVal = k.key.trimmed();
        if (!keyVal.isEmpty()) {
            keyCounts[keyVal]++;
        }
    }

    for (auto &k : m_keys) {
        QString keyVal = k.key.trimmed();
        k.isDuplicate = (keyCounts.value(keyVal, 0) > 1);
    }
}

void KeyPoolManager::addKey(const ApiKeyItem &item) {
    ApiKeyItem newKey = item;
    if (newKey.id.isEmpty()) {
        newKey.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    m_keys.append(newKey);
    updateDuplicates();
    saveToDisk();
    emit keysUpdated();

    // Auto-test newly added key
    testKey(newKey.id);
}

void KeyPoolManager::removeKey(const QString &id) {
    for (int i = 0; i < m_keys.size(); ++i) {
        if (m_keys[i].id == id) {
            m_keys.removeAt(i);
            break;
        }
    }
    updateDuplicates();
    saveToDisk();
    emit keysUpdated();
}

void KeyPoolManager::toggleKey(const QString &id, bool enabled) {
    for (auto &k : m_keys) {
        if (k.id == id) {
            k.enabled = enabled;
            k.status = enabled ? "Untested" : "Disabled";
            break;
        }
    }
    updateDuplicates();
    saveToDisk();
    emit keysUpdated();
}

void KeyPoolManager::setQueueStrategy(QueueStrategy strategy) {
    if (m_queueStrategy != strategy) {
        m_queueStrategy = strategy;
        saveToDisk();
        emit keysUpdated();
    }
}

void KeyPoolManager::moveKeyUp(int index) {
    if (index > 0 && index < m_keys.size()) {
        m_keys.swapItemsAt(index, index - 1);
        saveToDisk();
        emit keysUpdated();
    }
}

void KeyPoolManager::moveKeyDown(int index) {
    if (index >= 0 && index < m_keys.size() - 1) {
        m_keys.swapItemsAt(index, index + 1);
        saveToDisk();
        emit keysUpdated();
    }
}

void KeyPoolManager::loadFromDisk(const QString &filePath) {
    QString path = filePath.isEmpty() ? getDefaultConfigPath() : filePath;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray array;

    if (doc.isObject()) {
        QJsonObject root = doc.object();
        m_queueStrategy = static_cast<QueueStrategy>(root["queueStrategy"].toInt(0));
        array = root["keys"].toArray();
    } else if (doc.isArray()) {
        array = doc.array();
    } else {
        return;
    }

    m_keys.clear();
    for (const auto &val : array) {
        QJsonObject obj = val.toObject();
        ApiKeyItem item;
        item.id = obj["id"].toString();
        item.provider = obj["provider"].toString();
        item.alias = obj["alias"].toString();
        item.key = obj["key"].toString();
        item.status = obj["status"].toString("Untested");
        item.rpmLimit = obj["rpmLimit"].toInt(0);
        item.tpmLimit = obj["tpmLimit"].toInt(0);
        item.rpmRemaining = obj["rpmRemaining"].toInt(-1);
        item.tpmRemaining = obj["tpmRemaining"].toInt(-1);
        item.priority = obj["priority"].toString("Medium");
        item.enabled = obj["enabled"].toBool(true);

        // Sanitize legacy hardcoded 30 RPM / 1000000 TPM values from existing config
        if (item.rpmLimit == 30 && (item.tpmLimit == 1000000 || item.tpmLimit == 100000)) {
            item.rpmLimit = 0;
            item.tpmLimit = 0;
        }

        m_keys.append(item);
    }

    updateDuplicates();
    saveToDisk();
    emit keysUpdated();
}

void KeyPoolManager::saveToDisk(const QString &filePath) const {
    QString path = filePath.isEmpty() ? getDefaultConfigPath() : filePath;
    QJsonObject root;
    root["queueStrategy"] = static_cast<int>(m_queueStrategy);

    QJsonArray array;
    for (const auto &item : m_keys) {
        QJsonObject obj;
        obj["id"] = item.id;
        obj["provider"] = item.provider;
        obj["alias"] = item.alias;
        obj["key"] = item.key;
        obj["status"] = item.status;
        obj["rpmLimit"] = item.rpmLimit;
        obj["tpmLimit"] = item.tpmLimit;
        obj["rpmRemaining"] = item.rpmRemaining;
        obj["tpmRemaining"] = item.tpmRemaining;
        obj["priority"] = item.priority;
        obj["enabled"] = item.enabled;
        array.append(obj);
    }
    root["keys"] = array;

    QJsonDocument doc(root);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

void KeyPoolManager::updateKeyRateLimitFromHeaders(const QString &keyId, const QList<QNetworkReply::RawHeaderPair> &headers) {
    int keyIdx = -1;
    for (int i = 0; i < m_keys.size(); ++i) {
        if (m_keys[i].id == keyId) {
            keyIdx = i;
            break;
        }
    }
    if (keyIdx == -1) return;

    auto &k = m_keys[keyIdx];
    bool updated = false;

    for (const auto &pair : headers) {
        QString name = QString::fromUtf8(pair.first).toLower();
        QString val = QString::fromUtf8(pair.second).trimmed();

        if (name == "x-ratelimit-limit-requests" || name == "ratelimit-limit-requests" || 
            name == "x-ratelimit-requests-limit" || name == "anthropic-ratelimit-requests-limit") {
            int limit = val.toInt();
            if (limit > 0 && k.rpmLimit != limit) {
                k.rpmLimit = limit;
                updated = true;
            }
        } else if (name == "x-ratelimit-remaining-requests" || name == "ratelimit-remaining-requests" || 
                   name == "x-ratelimit-requests-remaining" || name == "anthropic-ratelimit-requests-remaining") {
            int rem = val.toInt();
            if (rem >= 0 && k.rpmRemaining != rem) {
                k.rpmRemaining = rem;
                updated = true;
            }
        } else if (name == "x-ratelimit-limit-tokens" || name == "ratelimit-limit-tokens" || 
                   name == "x-ratelimit-tokens-limit" || name == "anthropic-ratelimit-tokens-limit") {
            int limit = val.toInt();
            if (limit > 0 && k.tpmLimit != limit) {
                k.tpmLimit = limit;
                updated = true;
            }
        } else if (name == "x-ratelimit-remaining-tokens" || name == "ratelimit-remaining-tokens" || 
                   name == "x-ratelimit-tokens-remaining" || name == "anthropic-ratelimit-tokens-remaining") {
            int rem = val.toInt();
            if (rem >= 0 && k.tpmRemaining != rem) {
                k.tpmRemaining = rem;
                updated = true;
            }
        }
    }

    if (updated) {
        saveToDisk();
        emit keysUpdated();
    }
}

void KeyPoolManager::testKey(const QString &id) {
    int keyIdx = -1;
    for (int i = 0; i < m_keys.size(); ++i) {
        if (m_keys[i].id == id) {
            keyIdx = i;
            break;
        }
    }

    if (keyIdx == -1) return;

    ApiKeyItem &item = m_keys[keyIdx];

    // Anti-Spam Fuse: Ignore rapid clicks if a request is active
    if (item.status == "Testing...") {
        return;
    }

    // Cooldown Fuse: Prevent rapid ping spam within 3 seconds
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 COOLDOWN_MS = 3000;

    if (m_lastTestTimes.contains(id)) {
        qint64 elapsed = now - m_lastTestTimes[id];
        if (elapsed < COOLDOWN_MS) {
            int waitSec = static_cast<int>((COOLDOWN_MS - elapsed) / 1000) + 1;
            item.status = QString("Cooldown (%1s)...").arg(waitSec);
            emit keysUpdated();

            qint64 remain = COOLDOWN_MS - elapsed;
            QTimer::singleShot(remain, this, [this, id]() {
                for (auto &k : m_keys) {
                    if (k.id == id && k.status.startsWith("Cooldown")) {
                        k.status = "Active";
                        emit keysUpdated();
                        break;
                    }
                }
            });
            return;
        }
    }

    m_lastTestTimes[id] = now;
    item.status = "Testing...";
    emit keysUpdated();

    QNetworkRequest request;
    QString prov = item.provider.toLower();

    if (prov.contains("gemini")) {
        QUrl url(QString("https://generativelanguage.googleapis.com/v1beta/models?key=%1").arg(item.key));
        request.setUrl(url);
    } else if (prov.contains("groq")) {
        request.setUrl(QUrl("https://api.groq.com/openai/v1/models"));
        request.setRawHeader("Authorization", QString("Bearer %1").arg(item.key).toUtf8());
    } else if (prov.contains("openrouter")) {
        request.setUrl(QUrl("https://openrouter.ai/api/v1/auth/key"));
        request.setRawHeader("Authorization", QString("Bearer %1").arg(item.key).toUtf8());
    } else if (prov.contains("anthropic")) {
        request.setUrl(QUrl("https://api.anthropic.com/v1/models"));
        request.setRawHeader("x-api-key", item.key.toUtf8());
        request.setRawHeader("anthropic-version", "2023-06-01");
    } else {
        request.setUrl(QUrl("https://api.openai.com/v1/models"));
        request.setRawHeader("Authorization", QString("Bearer %1").arg(item.key).toUtf8());
    }

    QNetworkReply *reply = m_netManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, id]() {
        reply->deleteLater();
        updateKeyRateLimitFromHeaders(id, reply->rawHeaderPairs());

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        int targetIdx = -1;
        for (int i = 0; i < m_keys.size(); ++i) {
            if (m_keys[i].id == id) {
                targetIdx = i;
                break;
            }
        }

        if (targetIdx == -1) return;

        bool ok = false;
        QString statusStr;

        if (statusCode == 200) {
            ok = true;
            statusStr = "Active";
        } else if (statusCode == 429) {
            statusStr = "Rate Limited (429)";
        } else if (statusCode == 401 || statusCode == 403) {
            statusStr = "Invalid Key (401)";
        } else {
            if (reply->error() == QNetworkReply::NoError) {
                ok = true;
                statusStr = "Active";
            } else {
                statusStr = QString("Error (%1)").arg(statusCode > 0 ? QString::number(statusCode) : reply->errorString());
            }
        }

        m_keys[targetIdx].status = statusStr;
        saveToDisk();
        emit keysUpdated();
        emit keyStatusChanged(id, statusStr, ok);
    });
}

void KeyPoolManager::testAllKeys() {
    int delayMs = 0;
    for (const auto &k : m_keys) {
        if (k.enabled) {
            QString keyId = k.id;
            QTimer::singleShot(delayMs, this, [this, keyId]() {
                testKey(keyId);
            });
            delayMs += 250; // Stagger tests by 250ms per key to prevent burst rate-limiting
        }
    }
}

struct GeminiModelRank {
    QString name;
    int isPreview; // 0 for stable, 1 for preview/exp
    int negMajor;  // -major
    int negMinor;  // -minor
    int tier;      // 0 for flash, 1 for pro, 2 for other
    int nameLength;

    bool operator<(const GeminiModelRank &other) const {
        if (isPreview != other.isPreview) return isPreview < other.isPreview;
        if (negMajor != other.negMajor) return negMajor < other.negMajor;
        if (negMinor != other.negMinor) return negMinor < other.negMinor;
        if (tier != other.tier) return tier < other.tier;
        return nameLength < other.nameLength;
    }
};

static GeminiModelRank rankGeminiModelItem(const QString &modelName) {
    QString name = modelName.toLower();
    GeminiModelRank r;
    r.name = modelName;
    r.isPreview = (name.contains("preview") || name.contains("exp")) ? 1 : 0;
    r.tier = name.contains("flash") ? 0 : (name.contains("pro") ? 1 : 2);
    r.nameLength = name.length();

    static QRegularExpression re("gemini-(\\d+)(?:\\.(\\d+))?");
    QRegularExpressionMatch match = re.match(name);
    int major = match.hasMatch() ? match.captured(1).toInt() : 0;
    int minor = (match.hasMatch() && !match.captured(2).isEmpty()) ? match.captured(2).toInt() : 0;

    r.negMajor = -major;
    r.negMinor = -minor;
    return r;
}

void KeyPoolManager::discoverGeminiModels(const QString &keyStr) {
    if (keyStr.trimmed().isEmpty()) return;

    QUrl url(QString("https://generativelanguage.googleapis.com/v1beta/models?key=%1").arg(keyStr.trimmed()));
    QNetworkRequest req(url);

    QNetworkReply *reply = m_netManager->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, keyStr]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) return;

        QJsonObject obj = doc.object();
        QJsonArray modelsArray = obj["models"].toArray();

        QList<GeminiModelRank> rankedList;
        for (const auto &val : modelsArray) {
            QJsonObject mObj = val.toObject();
            QString name = mObj["name"].toString().replace("models/", "").trimmed();
            
            QJsonArray methods = mObj["supportedGenerationMethods"].toArray();
            bool supportsGen = false;
            for (const auto &method : methods) {
                if (method.toString() == "generateContent") {
                    supportsGen = true;
                    break;
                }
            }

            if (!supportsGen) continue;

            QString nameLower = name.toLower();
            if (nameLower.contains("-tts") || nameLower.contains("embedding") || nameLower.contains("imagen") || nameLower.contains("bison") || nameLower.contains("aqa")) {
                continue;
            }

            rankedList.append(rankGeminiModelItem(name));
        }

        std::sort(rankedList.begin(), rankedList.end());

        QStringList finalModels;
        for (const auto &item : rankedList) {
            finalModels.append(item.name);
        }

        if (!finalModels.isEmpty()) {
            m_geminiDiscoveredModels[keyStr.trimmed()] = finalModels;
        }
    });
}

QStringList KeyPoolManager::getGeminiModelCandidates(const QString &keyStr) {
    QString trimmed = keyStr.trimmed();
    if (m_geminiDiscoveredModels.contains(trimmed) && !m_geminiDiscoveredModels[trimmed].isEmpty()) {
        return m_geminiDiscoveredModels[trimmed];
    }
    discoverGeminiModels(trimmed);
    return {"gemini-3.6-flash", "gemini-2.5-flash", "gemini-1.5-flash"};
}

QString KeyPoolManager::getBestGeminiModel(const QString &keyStr) {
    QStringList candidates = getGeminiModelCandidates(keyStr);
    return candidates.isEmpty() ? "gemini-3.6-flash" : candidates.first();
}
