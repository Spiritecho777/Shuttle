#include <QLockFile>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>

#include "LockManager.h"

bool LockManager::acquire()
{
    QString lockPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Shuttle.lock";
    QDir().mkpath(QFileInfo(lockPath).absolutePath());

    static QLockFile lock(lockPath);
    lock.setStaleLockTime(0);

    if (!lock.tryLock()) {
        QMessageBox::warning(nullptr, QObject::tr("Instance déjà ouverte"),
            QObject::tr("Shuttle est déjà en cours d'exécution."));

        return false;
    }

    return true;
}