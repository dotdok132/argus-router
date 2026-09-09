#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>

class MemoryManager : public QObject {
    Q_OBJECT

public:
    explicit MemoryManager(QObject *parent = nullptr);

    QString getMemoryDirectory() const;
    QStringList listMemories() const;
    QString readMemory(const QString &fileName) const;
    bool saveMemory(const QString &fileName, const QString &content);
    bool createMemory(const QString &fileName, const QString &content = "");
    bool deleteMemory(const QString &fileName);

    void initDefaultMemories();

signals:
    void memoryListUpdated();
    void memorySaved(const QString &fileName);

private:
    QString m_memoryDir;
};

#endif // MEMORYMANAGER_H
