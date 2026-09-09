#include "MemoryManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>

MemoryManager::MemoryManager(QObject *parent) : QObject(parent) {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configDir);
    m_memoryDir = dir.filePath("memories");
    
    QDir memDir(m_memoryDir);
    if (!memDir.exists()) {
        memDir.mkpath(".");
    }

    initDefaultMemories();
}

QString MemoryManager::getMemoryDirectory() const {
    return m_memoryDir;
}

void MemoryManager::initDefaultMemories() {
    QDir dir(m_memoryDir);

    auto ensureFile = [this, &dir](const QString &fileName, const QString &defaultContent) {
        QString filePath = dir.filePath(fileName);
        if (!QFile::exists(filePath)) {
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << defaultContent;
                file.close();
            }
        }
    };

    ensureFile("user_character.md", 
        "# User Communication & Tone Preferences\n\n"
        "- Tone: Direct, concise, technical, no fluff.\n"
        "- Emojis: Strictly forbidden in UI text, logs, and answers.\n"
        "- Style: Clean GFM markdown output.\n"
    );

    ensureFile("code_requirements.md", 
        "# Code Architecture & Standards\n\n"
        "- Technology Stack: Modern C++ (C++17/C++20) & Qt6.\n"
        "- Design: Dark IDE theme (#1e1e1e, #252526, #007acc).\n"
        "- Robustness: No silent fallbacks, no dummy placeholders, exact traceback justifications.\n"
        "- Quality: Flat ghost buttons, precise QTableWidget column sizing.\n"
    );

    ensureFile("project_context.md", 
        "# Argus Token Router Context\n\n"
        "- Purpose: Local OpenAI-compatible proxy server listening on port 8080.\n"
        "- Dynamic Gemini engine: Automatic model discovery and fallback (gemini-3.6-flash / meta-llama/llama-3.3-70b-instruct).\n"
        "- Failover: Multi-key automatic failover pool with real-time rate limit header discovery.\n"
    );

    emit memoryListUpdated();
}

QStringList MemoryManager::listMemories() const {
    QDir dir(m_memoryDir);
    QStringList filters;
    filters << "*.md";
    return dir.entryList(filters, QDir::Files, QDir::Name);
}

QString MemoryManager::readMemory(const QString &fileName) const {
    QDir dir(m_memoryDir);
    QFile file(dir.filePath(fileName));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    QTextStream in(&file);
    return in.readAll();
}

bool MemoryManager::saveMemory(const QString &fileName, const QString &content) {
    QDir dir(m_memoryDir);
    QFile file(dir.filePath(fileName));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream out(&file);
    out << content;
    file.close();

    emit memorySaved(fileName);
    emit memoryListUpdated();
    return true;
}

bool MemoryManager::createMemory(const QString &fileName, const QString &content) {
    QString name = fileName;
    if (!name.endsWith(".md")) {
        name += ".md";
    }
    return saveMemory(name, content);
}

bool MemoryManager::deleteMemory(const QString &fileName) {
    QDir dir(m_memoryDir);
    bool ok = dir.remove(fileName);
    if (ok) {
        emit memoryListUpdated();
    }
    return ok;
}
