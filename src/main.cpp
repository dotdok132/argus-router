#include <QApplication>
#include <QCoreApplication>
#include "ui/MainWindow.h"
#include "core/KeyPoolManager.h"
#include "core/HttpProxyServer.h"
#include "core/MemoryManager.h"
#include <QDebug>

void customLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    Q_UNUSED(context);
    QByteArray localMsg = msg.toLocal8Bit();
    const char *prefix = (type == QtWarningMsg) ? "[WARN] " : ((type == QtCriticalMsg) ? "[ERR] " : "[INFO] ");
    fprintf(stdout, "%s%s\n", prefix, localMsg.constData());
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    qInstallMessageHandler(customLogHandler);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    bool headless = false;
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromUtf8(argv[i]);
        if (arg == "--cli" || arg == "--headless" || arg == "-h" || arg == "--daemon") {
            headless = true;
        }
    }

    if (headless) {
        QCoreApplication app(argc, argv);
        app.setApplicationName("QtTokenRouter");
        app.setOrganizationName("ArgusAI");

        KeyPoolManager poolMgr;
        HttpProxyServer proxyServer(&poolMgr);
        MemoryManager memMgr;
        proxyServer.setMemoryManager(&memMgr);

        if (proxyServer.start(8080)) {
            qDebug() << "[Argus Token Router] Daemon running headlessly on http://127.0.0.1:8080";
        } else {
            qCritical() << "[Argus Token Router] Failed to bind port 8080";
            return 1;
        }

        return app.exec();
    }

    QApplication app(argc, argv);
    app.setApplicationName("QtTokenRouter");
    app.setOrganizationName("ArgusAI");

    MainWindow window;
    window.show();

    return app.exec();
}
