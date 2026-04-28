#include "MonitorBar.h"

#include <QApplication>
#include <QStyle>

MonitorBar::MonitorBar(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    setFixedHeight(24);
}

MonitorBar::~MonitorBar()
{
    disconnectSession();
}

void MonitorBar::setupUi()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 0, 6, 0);
    layout->setSpacing(12);

    // Style global de la barre
    setStyleSheet(
        "MonitorBar {"
        "  background-color: #1e1e2e;"
        "  border-top: 1px solid #444;"
        "}"
        "QLabel {"
        "  color: #0E0E12;"
        "  font-size: 11px;"
        "  font-family: 'Consolas', 'Courier New', monospace;"
        "}"
    );

    // Host
    m_hostLabel = new QLabel("—", this);
    m_hostLabel->setStyleSheet("color: #038A05; font-weight: bold;");
    layout->addWidget(m_hostLabel);

    layout->addWidget(makeSeparator());

    // CPU
    auto* cpuIcon = new QLabel("⚙", this);
    layout->addWidget(cpuIcon);
    m_cpuLabel = new QLabel("CPU: —", this);
    layout->addWidget(m_cpuLabel);

    layout->addWidget(makeSeparator());

    // RAM
    auto* ramIcon = new QLabel("▣", this);
    layout->addWidget(ramIcon);
    m_ramLabel = new QLabel("RAM: —", this);
    layout->addWidget(m_ramLabel);

    layout->addWidget(makeSeparator());

    // Réseau
    auto* netIcon = new QLabel("⇅", this);
    layout->addWidget(netIcon);
    m_netLabel = new QLabel("↑— ↓—", this);
    layout->addWidget(m_netLabel);

    layout->addWidget(makeSeparator());

    // Disques
    auto* diskIcon = new QLabel("💾", this);
    layout->addWidget(diskIcon);
    m_diskLabel = new QLabel("Disques: —", this);
    layout->addWidget(m_diskLabel);

    layout->addStretch();
}

QWidget* MonitorBar::makeSeparator()
{
    auto* sep = new QLabel("│", this);
    sep->setStyleSheet("color: #444; margin: 0 2px;");
    return sep;
}

// -----------------------------------------------------------------
// Session
// -----------------------------------------------------------------

void MonitorBar::connectTo(const SessionProfile& profile)
{
    disconnectSession();

    m_hostLabel->setText(profile.host);

    m_session = new MonitorSession(profile, this);
    connect(m_session, &MonitorSession::dataUpdated,
        this, &MonitorBar::onDataUpdated);
    connect(m_session, &MonitorSession::connectionFailed,
        this, [this](const QString&) {
            m_hostLabel->setText(m_hostLabel->text() + " ⚠");
        });

    m_session->start();
}

void MonitorBar::disconnectSession()
{
    if (!m_session) return;
    m_session->stopMonitor();
    m_session->wait(2000);
    m_session->deleteLater();
    m_session = nullptr;

    // Reset affichage
    m_cpuLabel->setText("CPU: —");
    m_ramLabel->setText("RAM: —");
    m_netLabel->setText("↑— ↓—");
    m_diskLabel->setText("Disques: —");
}

// -----------------------------------------------------------------
// Mise à jour affichage
// -----------------------------------------------------------------

void MonitorBar::onDataUpdated(const MonitorData& data)
{
    if (!data.valid) return;

    // CPU — couleur selon charge
    QString cpuColor = data.cpuPercent > 80 ? "#FF0000"   // rouge
        : data.cpuPercent > 50 ? "#FF842E"   // orange
        : "#22D911";                          // vert
    m_cpuLabel->setText(QString("CPU: %1%").arg(static_cast<int>(data.cpuPercent)));
    m_cpuLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(cpuColor));

    // RAM
    float usedGb = static_cast<float>(data.memTotalKb - data.memAvailableKb) / 1024.0f / 1024.0f;
    float totalGb = static_cast<float>(data.memTotalKb) / 1024.0f / 1024.0f;

    QString ramColor = data.memPercent > 80 ? "#FF0000"
        : data.memPercent > 60 ? "#FF842E"
        : "#22D911";
    m_ramLabel->setText(QString("RAM: %1/%2 GB")
        .arg(usedGb, 0, 'f', 2)
        .arg(totalGb, 0, 'f', 2));
    m_ramLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(ramColor));

    // Réseau
    m_netLabel->setText(QString("↑%1 ↓%2")
        .arg(formatBytes(data.txBytesPerSec))
        .arg(formatBytes(data.rxBytesPerSec)));

    // Disques
    if (!data.disks.isEmpty()) {
        QStringList parts;
        for (const DiskInfo& d : data.disks) {
            QString color = d.usedPercent > 90 ? "#FF0000"
                : d.usedPercent > 70 ? "#FF842E"
                : "#22D911";
            parts += QString("<span style='color:%1'>%2: %3%</span>")
                .arg(color, d.mountPoint, QString::number(d.usedPercent));
        }
        m_diskLabel->setText(parts.join("  "));
        m_diskLabel->setTextFormat(Qt::RichText);
    }
}

QString MonitorBar::formatBytes(float bytesPerSec) const
{
    if (bytesPerSec < 1024)
        return QString("%1 B/s").arg(static_cast<int>(bytesPerSec));
    if (bytesPerSec < 1024 * 1024)
        return QString("%1 KB/s").arg(bytesPerSec / 1024.0f, 0, 'f', 1);
    return QString("%1 MB/s").arg(bytesPerSec / 1024.0f / 1024.0f, 0, 'f', 2);
}