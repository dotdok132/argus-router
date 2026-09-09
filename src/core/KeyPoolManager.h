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
    int rpmLimit = 30;
    int tpmLimit = 1000000;
    QString priority = "Medium";
    bool enabled = true;
    int currentRpm = 0;
    bool isDuplicate = false;
};

class KeyPoolManager : public QObject {
    Q_OBJECT

public:
    explicit KeyPoolManager(QObject *parent = nullptr);

    const QList<ApiKeyItem>& getKeys() const { return m_keys; }
    void addKey(const ApiKeyItem &item);
    void removeKey(const QString &id);
    void toggleKey(const QString &id, bool enabled);

    bool isDuplicateKey(const QString &keyStr, const QString &ignoreId = "") const;

    void loadFromDisk(const QString &filePath = "");
    void saveToDisk(const QString &filePath = "") const;

    void testKey(const QString &id);
    void testAllKeys();

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
    QNetworkAccessManager *m_netManager;
    QHash<QString, qint64> m_lastTestTimes; // Anti-spam test fuse (timestamp in msecs)
    QHash<QString, QStringList> m_geminiDiscoveredModels;
};

#endif // KEYPOOLMANAGER_H
