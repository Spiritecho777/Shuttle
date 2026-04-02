#include <QCoreApplication>
#include <QApplication>
#include <QIcon>

#include <libssh2.h>

#include "core/AppInitializer.h"
#include "ui/ShuttleWindow.h"
#include "ui/TrayManager.h"

int main(int argc, char* argv[])
{
	libssh2_init(0);

    QCoreApplication::setOrganizationName("Stusoft");
    QCoreApplication::setApplicationName("Shuttle");

    QApplication a(argc, argv);
    a.setApplicationName("Shuttle");
    a.setWindowIcon(QIcon(QCoreApplication::applicationDirPath() + "/Asset/Icone.png"));
    a.setQuitOnLastWindowClosed(false);

    if (!AppInitializer::init())
        return 0;

    ShuttleWindow w;
    //TrayManager tray(&w);

    w.show();

    int ret = a.exec();
	libssh2_exit();
    return ret;
}