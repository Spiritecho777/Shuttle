#pragma once

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <atomic>

#include <libssh2.h>

#ifdef _WIN32
#include <winsock2.h>
#endif

#include "../../ssh/SessionProfile.h"
#include "MonitorData.h"

class MonitorSession : public QThread
{
    Q_OBJECT

public:
    explicit MonitorSession(const SessionProfile& profile, QObject* parent = nullptr);
    ~MonitorSession();

    void stopMonitor();

signals:
    void dataUpdated(const MonitorData& data);
    void connectionFailed(const QString& error);

protected:
    void run() override;

private:
    bool initSocket();
    bool initSsh();
    void cleanup();

    // Exécute une commande via SSH exec et retourne stdout
    QByteArray execCommand(const QString& cmd);

    // Parse la sortie brute en MonitorData
    MonitorData parseOutput(const QByteArray& raw);

    // CPU — nécessite deux mesures pour calculer le %
    struct CpuSnapshot {
        quint64 user = 0, nice = 0, system = 0, idle = 0,
            iowait = 0, irq = 0, softirq = 0;
        quint64 total()  const { return user + nice + system + idle + iowait + irq + softirq; }
        quint64 active() const { return user + nice + system + irq + softirq; }
    };

    float calcCpuPercent(const CpuSnapshot& prev, const CpuSnapshot& curr);
    CpuSnapshot parseCpuLine(const QByteArray& line);

    // Réseau — idem, deux mesures pour le débit
    struct NetSnapshot {
        quint64 rx = 0;
        quint64 tx = 0;
    };
    NetSnapshot parseNetBytes(const QByteArray& output);

    SessionProfile   m_profile;
	std::atomic<bool> m_running{false};

    // État précédent pour les calculs de delta
    CpuSnapshot m_prevCpu;
    NetSnapshot m_prevNet;
    bool        m_firstRun = true;

#ifdef _WIN32
    SOCKET m_sock = INVALID_SOCKET;
#else
    int m_sock = -1;
#endif

    LIBSSH2_SESSION* m_session = nullptr;
};