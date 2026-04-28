#pragma once

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QProgressBar>

#include "../../ssh/SessionProfile.h"
#include "MonitorData.h"
#include "MonitorSession.h"

class MonitorBar : public QWidget
{
    Q_OBJECT

public:
    explicit MonitorBar(QWidget* parent = nullptr);
    ~MonitorBar();

    void connectTo(const SessionProfile& profile);
    void disconnectSession();

private slots:
    void onDataUpdated(const MonitorData& data);

private:
    void setupUi();
    QString formatBytes(float bytesPerSec) const;

    // Session de monitoring
    MonitorSession* m_session = nullptr;

    // Labels
    QLabel* m_cpuLabel = nullptr;
    QLabel* m_ramLabel = nullptr;
    QLabel* m_netLabel = nullptr;
    QLabel* m_diskLabel = nullptr;
    QLabel* m_hostLabel = nullptr;

	QWidget* makeSeparator();
};