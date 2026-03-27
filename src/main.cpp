#include <QApplication>
#include <QIcon>

#include "core/AppInitializer.h"
#include "ui/ShuttleWindow.h"
#include "ui/TrayManager.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Shuttle");
    a.setWindowIcon(QIcon(QCoreApplication::applicationDirPath() + "/Asset/Icone.png"));
    a.setQuitOnLastWindowClosed(false);

    if (!AppInitializer::init())
        return 0;

    ShuttleWindow w;
    TrayManager tray(&w);

    w.show();

    return a.exec();
}