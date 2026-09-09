#ifndef KEYPOOLMANAGER_H
#define KEYPOOLMANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <QHash>

struct ApiKeyItem {
    QString id;
    QString provider;    // gemini, groq, openrouter, anthropic, custom
    QString alias;
    QString key;
    QString status;      // Active, Cooldown, Invalid, Disabled, Untested, Duplicate
    int rpmLimit = 0;    // 0 = Auto-discovered from response headers
    int tpmLimit = 0;    // 0 = Auto-discovered from response headers
    int rpmRemaining = -1;
    int tpmRemaining = -1;
    qint64 totalTokensUsed = 0;
    QString priority = "Medium";
    bool enabled = true;
    int currentRpm = 0;
    bool isDuplicate = false;
};

enum class QueueStrategy {
    SequentialPriority = 0, // Top-to-bottom failover queue
    RoundRobin = 1,          // Cyclic load distribution
    LeastLoaded = 2          // Capacity-based selection
};

class KeyPoolManager : public QObject {
    Q_OBJECT

public:
    explicit KeyPoolManager(QObject *parent = nullptr);

    const QList<ApiKeyItem>& getKeys() const { return m_keys; }
    void addKey(const ApiKeyItem &item);
    void removeKey(const QString &id);
    void toggleKey(const QString &id, bool enabled);

    QueueStrategy getQueueStrategy() const { return m_queueStrategy; }
    void setQueueStrategy(QueueStrategy strategy);

    void moveKeyUp(int index);
    void moveKeyDown(int index);

    void recordTokenUsage(const QString &keyId, qint64 tokens);
    qint64 getTotalTokensServed() const;

    bool isDuplicateKey(const QString &keyStr, const QString &ignoreId = "") const;

    void loadFromDisk(const QString &filePath = "");
    void saveToDisk(const QString &filePath = "") const;

    void testKey(const QString &id);
    void testAllKeys();

    void updateKeyRateLimitFromHeaders(const QString &keyId, const QList<QNetworkReply::RawHeaderPair> &headers);

    QString getBestGeminiModel(const QString &keyStr);
    QStringList getGeminiModelCandidates(const QString &keyStr);

signals:
    void keysUpdated();
    void keyStatusChanged(const QString &id, const QString &status, bool ok);

private:
    void updateDuplicates();
    void discoverGeminiModels(const QString &keyStr);
    QString getDefaultConfigPath() const;

    QList<ApiKeyItem> m_keys;
    QueueStrategy m_queueStrategy = QueueStrategy::SequentialPriority;
    QNetworkAccessManager *m_netManager;
    QHash<QString, qint64> m_lastTestTimes; // Anti-spam test fuse (timestamp in msecs)
    QHash<QString, QStringList> m_geminiDiscoveredModels;
};

#endif // KEYPOOLMANAGER_H
