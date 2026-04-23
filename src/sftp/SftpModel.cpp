#include "SftpModel.h"
#include <QStyle>
#include <QApplication>

SftpModel::SftpModel(QObject* parent)
    : QAbstractItemModel(parent)
{
    // Icônes système — cross-platform
    m_iconDir = QApplication::style()->standardIcon(QStyle::SP_DirIcon);
    m_iconFile = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
}

void SftpModel::setEntries(const QString& path, const QList<SftpEntry>& entries)
{
    beginResetModel();
    m_currentPath = path;
    m_entries = entries;
    endResetModel();
}

SftpEntry SftpModel::entry(const QModelIndex& index) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};
    return m_entries[index.row()];
}

// -----------------------------------------------------------------
// QAbstractItemModel — liste plate (pas d'arbre)
// -----------------------------------------------------------------

QModelIndex SftpModel::index(int row, int column, const QModelIndex& parent) const
{
    if (parent.isValid()) return {};
    return createIndex(row, column);
}

QModelIndex SftpModel::parent(const QModelIndex&) const
{
    return {};
}

int SftpModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return m_entries.size();
}

int SftpModel::columnCount(const QModelIndex&) const
{
    return 4; // Nom | Taille | Modifié | Permissions
}

QVariant SftpModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};

    const SftpEntry& e = m_entries[index.row()];

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0: return e.name;
        case 1: return e.isDir ? QString("") : formatSize(e.size);
        case 2: return e.modified.isValid()
            ? e.modified.toString("yyyy-MM-dd hh:mm")
            : QString("");
        case 3: return formatPermissions(e.permissions);
        }
        break;

    case Qt::DecorationRole:
        if (index.column() == 0)
            return e.isDir ? m_iconDir : m_iconFile;
        break;

    case Qt::TextAlignmentRole:
        if (index.column() == 1)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        break;

    case Qt::UserRole:
        // Retourne l'entrée complète pour usage interne
        return QVariant::fromValue(index.row());
    }

    return {};
}

QVariant SftpModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section) {
    case 0: return "Nom";
    case 1: return "Taille";
    case 2: return "Modifié";
    case 3: return "Permissions";
    }
    return {};
}

// -----------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------

QString SftpModel::formatSize(quint64 bytes) const
{
    if (bytes < 1024)
        return QString("%1 o").arg(bytes);
    if (bytes < 1024 * 1024)
        return QString("%1 Ko").arg(bytes / 1024);
    if (bytes < 1024 * 1024 * 1024)
        return QString("%1 Mo").arg(bytes / (1024 * 1024));
    return QString("%1 Go").arg(bytes / (1024 * 1024 * 1024));
}

QString SftpModel::formatPermissions(uint perms) const
{
    if (perms == 0) return {};

    QString s;
    s += (perms & 0400) ? 'r' : '-';
    s += (perms & 0200) ? 'w' : '-';
    s += (perms & 0100) ? 'x' : '-';
    s += (perms & 0040) ? 'r' : '-';
    s += (perms & 0020) ? 'w' : '-';
    s += (perms & 0010) ? 'x' : '-';
    s += (perms & 0004) ? 'r' : '-';
    s += (perms & 0002) ? 'w' : '-';
    s += (perms & 0001) ? 'x' : '-';
    return s;
}