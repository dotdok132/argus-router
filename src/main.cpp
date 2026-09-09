#include <QApplication>
#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("QtTokenRouter");
    app.setOrganizationName("ArgusAI");

    MainWindow window;
    window.show();

    return app.exec();
}
