#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QSignalSpy>

#include "core/MemoryManager.h"
#include "core/KeyPoolManager.h"
#include "core/HttpProxyServer.h"

class ArgusRouterTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // MemoryManager Tests
    void testMemoryManagerDefaultFiles();
    void testMemoryManagerReadWrite();
    void testMemoryManagerListing();

    // KeyPoolManager Tests
    void testKeyPoolAddAndDuplicateDetection();
    void testKeyPoolTokenUsage();
    void testKeyPoolQueueStrategy();

    // HttpProxyServer Tests
    void testProxyServerListen();
};

void ArgusRouterTests::initTestCase() {
    qDebug() << "[Argus Test Suite] Initializing QtTest Suite...";
}

void ArgusRouterTests::cleanupTestCase() {
    qDebug() << "[Argus Test Suite] Cleanup complete.";
}

void ArgusRouterTests::testMemoryManagerDefaultFiles() {
    MemoryManager memMgr(nullptr);
    QStringList files = memMgr.listMemories();

    QVERIFY(files.contains("user_character.md"));
    QVERIFY(files.contains("code_requirements.md"));
    QVERIFY(files.contains("project_context.md"));
}

void ArgusRouterTests::testMemoryManagerReadWrite() {
    MemoryManager memMgr(nullptr);
    
    QString testContent = "# Test Title\nThis is test content.";
    QVERIFY(memMgr.saveMemory("custom_notes.md", testContent));

    QString readBack = memMgr.readMemory("custom_notes.md");
    QCOMPARE(readBack, testContent);
}

void ArgusRouterTests::testMemoryManagerListing() {
    MemoryManager memMgr(nullptr);
    memMgr.saveMemory("alpha.md", "alpha");
    memMgr.saveMemory("beta.md", "beta");

    QStringList list = memMgr.listMemories();
    QVERIFY(list.contains("alpha.md"));
    QVERIFY(list.contains("beta.md"));
}

void ArgusRouterTests::testKeyPoolAddAndDuplicateDetection() {
    KeyPoolManager poolMgr(nullptr);

    ApiKeyItem k1;
    k1.alias = "Test Key 1";
    k1.provider = "Google Gemini";
    k1.key = "AQ.TestKeyString12345";
    k1.enabled = true;
    k1.priority = "High";

    poolMgr.addKey(k1);
    QVERIFY(poolMgr.isDuplicateKey("AQ.TestKeyString12345"));
    QVERIFY(!poolMgr.isDuplicateKey("AQ.DifferentKey99999"));
}

void ArgusRouterTests::testKeyPoolTokenUsage() {
    KeyPoolManager poolMgr(nullptr);

    ApiKeyItem k1;
    k1.id = "test-key-id-100";
    k1.alias = "Test Usage Key";
    k1.provider = "Groq Speed Pool";
    k1.key = "gsk_test123";
    k1.enabled = true;

    poolMgr.addKey(k1);
    poolMgr.recordTokenUsage("test-key-id-100", 500);

    const auto &keys = poolMgr.getKeys();
    bool found = false;
    for (const auto &k : keys) {
        if (k.id == "test-key-id-100") {
            QCOMPARE(k.totalTokensUsed, static_cast<qint64>(500));
            found = true;
            break;
        }
    }
    QVERIFY(found);
}

void ArgusRouterTests::testKeyPoolQueueStrategy() {
    KeyPoolManager poolMgr(nullptr);
    poolMgr.setQueueStrategy(QueueStrategy::RoundRobin);
    QCOMPARE(poolMgr.getQueueStrategy(), QueueStrategy::RoundRobin);

    poolMgr.setQueueStrategy(QueueStrategy::LeastLoaded);
    QCOMPARE(poolMgr.getQueueStrategy(), QueueStrategy::LeastLoaded);
}

void ArgusRouterTests::testProxyServerListen() {
    KeyPoolManager poolMgr(nullptr);
    HttpProxyServer proxy(&poolMgr, nullptr);

    // Listen on testing port 18088
    bool ok = proxy.start(18088);
    QVERIFY(ok);
    proxy.stop();
}

QTEST_MAIN(ArgusRouterTests)
#include "test_main.moc"
