#include <QApplication>
#include <QIcon>

#include "core/AppInitializer.h"
#include "ui/ShuttleWindow.h"
#include "ui/TrayManager.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Shuttle");
    a.setWindowIcon(QIcon("/icons/Asset/Icone.png"));

    if (!AppInitializer::init())
        return 0;

    ShuttleWindow w;
    TrayManager tray(&w);

    w.show();

    return a.exec();
}