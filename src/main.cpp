#include <QCoreApplication>
#include <QApplication>
#include <QIcon>

#include <libssh2.h>

#include "core/AppInitializer.h"
#include "ui/ShuttleWindow.h"
#include "ui/TrayManager.h"

#ifdef _WIN32
#include <winsock2.h>
#endif

int main(int argc, char* argv[])
{
	libssh2_init(0);

#ifdef _WIN32
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif

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

#ifdef _WIN32
    WSACleanup();
#endif

    return ret;
}