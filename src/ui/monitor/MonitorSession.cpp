#include "MonitorSession.h"

#include <QDebug>
#include <QRegularExpression>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

// Commande unique qui collecte tout en une passe
static const char* kCollectCmd =
"echo '===CPU==='; cat /proc/stat | grep '^cpu '; "
"echo '===MEM==='; cat /proc/meminfo | grep -E '^(MemTotal|MemAvailable):'; "
"echo '===NET==='; cat /proc/net/dev; "
"echo '===DISK==='; /bin/df -h --output=target,pcent 2>/dev/null | tail -n +2";

MonitorSession::MonitorSession(const SessionProfile& profile, QObject* parent)
    : QThread(parent), m_profile(profile)
{}

MonitorSession::~MonitorSession()
{
    stopMonitor();
    wait(3000);
}

void MonitorSession::stopMonitor()
{
    m_running.store(false);
    // Réveille le thread s'il dort
    this->requestInterruption();
}

// -----------------------------------------------------------------
// Thread principal
// -----------------------------------------------------------------

void MonitorSession::run()
{
    m_running.store(true);

#ifdef _WIN32
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        emit connectionFailed("Monitor WSAStartup failed");
        return;
    }
#endif

    if (!initSocket() || !initSsh()) {
        cleanup();
        return;
    }

    // Boucle de polling toutes les 2 secondes
    while (m_running.load() && !isInterruptionRequested()) {

        QByteArray raw = execCommand(kCollectCmd);

        if (raw.isEmpty()) {
            // Connexion perdue — on arrête
            break;
        }

        MonitorData data = parseOutput(raw);
        data.valid = true;
        emit dataUpdated(data);

        // Attente 2s interruptible
        for (int i = 0; i < 20 && m_running.load(); ++i)
            msleep(100);
    }

    cleanup();
}

// -----------------------------------------------------------------
// SSH exec — ouvre un channel, exécute cmd, retourne stdout
// -----------------------------------------------------------------

QByteArray MonitorSession::execCommand(const QString& cmd)
{
    LIBSSH2_CHANNEL* channel = nullptr;

    // Ouvre un channel exec
    do {
        channel = libssh2_channel_open_session(m_session);
        if (!channel) {
            int err = libssh2_session_last_errno(m_session);
            if (err == LIBSSH2_ERROR_EAGAIN) { msleep(10); continue; }
            return {};
        }
    } while (!channel);

    // Exécute la commande
    int rc;
    QByteArray cmdUtf8 = cmd.toUtf8();
    do {
        rc = libssh2_channel_exec(channel, cmdUtf8.constData());
        if (rc == LIBSSH2_ERROR_EAGAIN) msleep(10);
    } while (rc == LIBSSH2_ERROR_EAGAIN);

    if (rc != 0) {
        libssh2_channel_free(channel);
        return {};
    }

    // Lit stdout
    QByteArray result;
    std::vector<char> buf(4096, 0);
    long nread;

    while (true) {
        nread = libssh2_channel_read(channel, buf.data(), buf.size());
        if (nread > 0) {
            result.append(buf.data(), static_cast<int>(nread));
        }
        else if (nread == LIBSSH2_ERROR_EAGAIN) {
            if (libssh2_channel_eof(channel)) break;
            msleep(10);
        }
        else {
            break; // erreur ou fin
        }
    }

    libssh2_channel_close(channel);
    libssh2_channel_free(channel);

    qDebug() << "=== RAW OUTPUT ===";
    qDebug() << QString::fromUtf8(result);

    return result;
}

// -----------------------------------------------------------------
// Parsing
// -----------------------------------------------------------------

MonitorData MonitorSession::parseOutput(const QByteArray& raw)
{
    MonitorData data;
    QString output = QString::fromUtf8(raw);

    // Extrait chaque section par regex simple
    auto extractSection = [&](const QString& name) -> QString {
        QString marker = "===" + name + "===";
        int start = output.indexOf(marker);
        if (start < 0) return {};
        start += marker.length();

        // Cherche la prochaine section ou fin
        int end = output.indexOf("===", start);
        if (end < 0) end = output.length();
        return output.mid(start, end - start).trimmed();
        };

    QString cpuSection = extractSection("CPU");
    QString memSection = extractSection("MEM");
    QString netSection = extractSection("NET");
    QString diskSection = extractSection("DISK");

    // --- CPU ---
    for (const QString& line : cpuSection.split('\n', Qt::SkipEmptyParts)) {
        if (line.startsWith("cpu ")) {
            CpuSnapshot curr = parseCpuLine(line.toUtf8());
            if (!m_firstRun)
                data.cpuPercent = calcCpuPercent(m_prevCpu, curr);
            m_prevCpu = curr;
            break;
        }
    }

    // --- RAM ---
    quint64 memTotal = 0, memAvail = 0;
    for (const QString& line : memSection.split('\n', Qt::SkipEmptyParts)) {
        QStringList parts = line.split(':', Qt::SkipEmptyParts);
        if (parts.size() < 2) continue;
        quint64 val = parts[1].trimmed().split(' ').first().toULongLong();
        if (parts[0].trimmed() == "MemTotal")     memTotal = val;
        if (parts[0].trimmed() == "MemAvailable") memAvail = val;
    }
    data.memTotalKb = memTotal;
    data.memAvailableKb = memAvail;
    if (memTotal > 0)
        data.memPercent = static_cast<float>(memTotal - memAvail) / memTotal * 100.0f;

    // --- Réseau ---
    NetSnapshot currNet = parseNetBytes(netSection.toUtf8());
    if (!m_firstRun) {
        data.rxBytesPerSec = static_cast<float>(
            currNet.rx > m_prevNet.rx ? currNet.rx - m_prevNet.rx : 0) / 2.0f;
        data.txBytesPerSec = static_cast<float>(
            currNet.tx > m_prevNet.tx ? currNet.tx - m_prevNet.tx : 0) / 2.0f;
    }
    m_prevNet = currNet;

    // --- Disques ---
    for (const QString& line : diskSection.split('\n', Qt::SkipEmptyParts)) {
        QString l = line.trimmed();
        if (l.isEmpty()) continue;
        QStringList parts = l.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() < 2) continue;

        QString mount = parts[0];
        QString pctStr = parts[1];
        pctStr.remove('%');

        if (mount.startsWith("/proc") || mount.startsWith("/sys") ||
            mount.startsWith("/dev") || mount.startsWith("/run")) continue;

        DiskInfo disk;
        disk.mountPoint = mount;
        disk.usedPercent = pctStr.toInt();
        data.disks.append(disk);
    }

    m_firstRun = false;
    return data;
}

MonitorSession::CpuSnapshot MonitorSession::parseCpuLine(const QByteArray& line)
{
    CpuSnapshot s;
    // Format: "cpu  user nice system idle iowait irq softirq ..."
    QList<QByteArray> parts = line.split(' ');
    // Retire les espaces vides
    parts.removeAll("");

    if (parts.size() >= 8) {
        s.user = parts[1].toULongLong();
        s.nice = parts[2].toULongLong();
        s.system = parts[3].toULongLong();
        s.idle = parts[4].toULongLong();
        s.iowait = parts[5].toULongLong();
        s.irq = parts[6].toULongLong();
        s.softirq = parts[7].toULongLong();
    }
    return s;
}

float MonitorSession::calcCpuPercent(const CpuSnapshot& prev, const CpuSnapshot& curr)
{
    quint64 totalDelta = curr.total() - prev.total();
    quint64 activeDelta = curr.active() - prev.active();
    if (totalDelta == 0) return 0.0f;
    return static_cast<float>(activeDelta) / static_cast<float>(totalDelta) * 100.0f;
}

MonitorSession::NetSnapshot MonitorSession::parseNetBytes(const QByteArray& output)
{
    NetSnapshot snap;
    QStringList lines = QString::fromUtf8(output).split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        QString l = line.trimmed();
        if (l.isEmpty() || l.startsWith("Inter") || l.startsWith("face"))
            continue;

        // Format: "eth0: rx_bytes ... tx_bytes ..."
        int colon = l.indexOf(':');
        if (colon < 0) continue;

        QString iface = l.left(colon).trimmed();
        if (iface == "lo") continue;  // ignore loopback

        QStringList parts = l.mid(colon + 1).trimmed().split(
            QRegularExpression("\\s+"), Qt::SkipEmptyParts);

        if (parts.size() >= 9) {
            snap.rx += parts[0].toULongLong();  // colonne 1 = rx bytes
            snap.tx += parts[8].toULongLong();  // colonne 9 = tx bytes
        }
    }
    return snap;
}

// -----------------------------------------------------------------
// Init connexion
// -----------------------------------------------------------------

bool MonitorSession::initSocket()
{
    m_sock = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (m_sock == INVALID_SOCKET) {
#else
    if (m_sock < 0) {
#endif
        emit connectionFailed("Monitor : impossible de créer le socket");
        return false;
    }

    sockaddr_in sin{};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(static_cast<u_short>(m_profile.port));

#ifdef _WIN32
    if (InetPtonA(AF_INET, m_profile.host.toUtf8().constData(), &sin.sin_addr) != 1) {
#else
    if (inet_pton(AF_INET, m_profile.host.toUtf8().constData(), &sin.sin_addr) != 1) {
#endif
        emit connectionFailed("Monitor : adresse IP invalide");
        return false;
    }

    if (::connect(m_sock, reinterpret_cast<sockaddr*>(&sin), sizeof(sin)) != 0) {
        emit connectionFailed("Monitor : connexion refusée");
        return false;
    }

    return true;
    }

bool MonitorSession::initSsh()
{
    m_session = libssh2_session_init();
    if (!m_session) {
        emit connectionFailed("Monitor : impossible d'initialiser SSH");
        return false;
    }

    libssh2_session_set_blocking(m_session, 1);

    if (libssh2_session_handshake(m_session, m_sock) != 0) {
        emit connectionFailed("Monitor : handshake SSH échoué");
        return false;
    }

    int rc = LIBSSH2_ERROR_AUTHENTICATION_FAILED;

    if (m_profile.authMethod == AuthMethod::Password) {
        rc = libssh2_userauth_password(
            m_session,
            m_profile.username.toUtf8().constData(),
            m_profile.password.toUtf8().constData()
        );
    }
    else {
        QByteArray passUtf8 = m_profile.passphrase.toUtf8();
        rc = libssh2_userauth_publickey_fromfile(
            m_session,
            m_profile.username.toUtf8().constData(),
            nullptr,
            m_profile.privateKeyPath.toUtf8().constData(),
            m_profile.passphrase.isEmpty() ? nullptr : passUtf8.constData()
        );
    }

    if (rc != 0) {
        char* errmsg = nullptr;
        libssh2_session_last_error(m_session, &errmsg, nullptr, 0);
        emit connectionFailed(QString("Monitor : auth échouée : %1").arg(errmsg));
        return false;
    }

    // Non-bloquant pour la boucle de lecture
    libssh2_session_set_blocking(m_session, 1);
    return true;
}

void MonitorSession::cleanup()
{
    if (m_session) {
        libssh2_session_disconnect(m_session, "Normal Shutdown");
        libssh2_session_free(m_session);
        m_session = nullptr;
    }
#ifdef _WIN32
    if (m_sock != INVALID_SOCKET) { closesocket(m_sock); m_sock = INVALID_SOCKET; }
    WSACleanup();
#else
    if (m_sock >= 0) { ::close(m_sock); m_sock = -1; }
#endif
}