#pragma once

#include <QString>
#include <QList>

struct DiskInfo {
    QString mountPoint;
    int     usedPercent = 0;  // 0-100
};

struct MonitorData {
    // CPU — usage en %
    float cpuPercent = 0.0f;

    // RAM
    quint64 memTotalKb = 0;
    quint64 memAvailableKb = 0;
    float   memPercent = 0.0f;  // (total - available) / total * 100

    // Réseau — delta par seconde calculé dans MonitorSession
    float rxBytesPerSec = 0.0f;
    float txBytesPerSec = 0.0f;

    // Disques
    QList<DiskInfo> disks;

    bool valid = false;  // false si la collecte a échoué
};