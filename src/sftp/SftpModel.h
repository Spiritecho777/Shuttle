#pragma once

#include <QAbstractItemModel>
#include <QList>
#include <QIcon>

#include "SftpSession.h"

class SftpModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit SftpModel(QObject* parent = nullptr);

    // Met à jour le contenu affiché
    void setEntries(const QString& path, const QList<SftpEntry>& entries);

    // Retourne l'entrée à un index donné
    SftpEntry entry(const QModelIndex& index) const;

    QString currentPath() const { return m_currentPath; }

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    QString           m_currentPath;
    QList<SftpEntry>  m_entries;

    QIcon m_iconDir;
    QIcon m_iconFile;

    QString formatSize(quint64 bytes) const;
    QString formatPermissions(uint perms) const;
};